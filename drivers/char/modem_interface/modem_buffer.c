/*
 *  kernel/driver/char/modem_interface/modem_buffer.c
 *
 *  Generic modem interface pingpang buffer API.
 *
 *  Author:     Jiayong Yang(Jiayong.Yang@spreadtrum.com)
 *  Created:    Jul 27, 2012
 *  Copyright:  Spreadtrum Inc.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 */
/* #define DEBUG */
/* #define VERBOSE_DEBUG */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/slab.h>
#include <mach/modem_interface.h>
#include <linux/kernel.h>
#include <linux/utsname.h>
#include <linux/semaphore.h>
#include <linux/irqflags.h>
#include <asm/uaccess.h>
#include "modem_buffer.h"
#define dloader_record_timestamp(time)
static int buffer_data_size(struct single_buffer_t *buffer)
{
	int data_size = 0;
	
	if (buffer->write_point == buffer->read_point)
		return 0;
	data_size = buffer->write_point - buffer->read_point;
	if (buffer->write_point < buffer->read_point)
		data_size = buffer->size + data_size;
	
	return data_size;
}

int save_to_receive_buffer(struct modem_buffer *buffer,char *data,int size)
{
	int free_space = 0;	
	int data_size = 0;
	int i;
	int write_point= buffer->buffer[0].write_point;

	if (buffer->type==BUF_RECV) {
		do{
			data_size = buffer_data_size(&buffer->buffer[0]);	
			free_space = buffer->buffer[0].size-1-data_size;
			if (free_space >= size)
				break;
			down(&buffer->buf_read_sem);
		}while(1);
		for(i=0;i<size;i++){
			buffer->buffer[0].addr[write_point++]=data[i];
			if(write_point >= buffer->buffer[0].size)
				write_point = 0;
		}
		buffer->buffer[0].write_point = write_point;
		up(&buffer->buf_read_sem);
		return size;
	}
	return 0;
}

int pingpang_buffer_read(struct modem_buffer *buffer,char __user *data,int size)
{
	int read_len = 0;
	int data_size = 0;	
	int i;
	int read_point= buffer->buffer[0].read_point;

	if (buffer->type==BUF_RECV) {
		do{
			if (buffer->buffer[0].write_point != buffer->buffer[0].read_point)
				break;
			down(&buffer->buf_read_sem);
		}while(1);

		data_size = buffer_data_size(&buffer->buffer[0]);

		if(data_size==0)
			return 0;

		if(data_size < size){
			read_len = data_size;
		} else {
			read_len = size;
		}
                if(copy_to_user(data,&buffer->buffer[0].addr[read_point],read_len)){
			printk("copy_to_user(%d,%d,%d) failed\n",read_len,size,data_size);
                        up(&buffer->buf_read_sem);
                        return -EFAULT;
                }

		buffer->buffer[0].read_point = 0; 
		buffer->buffer[0].write_point = 0; 
		up(&buffer->buf_read_sem);
		return read_len;
	}
	return 0;
}

int pingpang_buffer_send(struct modem_buffer *buffer)
{
	enum BUF_status_t status;
	int i,index;
	unsigned long flags;

	if(buffer->type == BUF_SEND){
		if(buffer->trans_index!=0xFF)
			index = buffer->trans_index;
		else
			index = buffer->save_index;
		local_irq_save(flags);
		for(i=0;i<2;i++){
			status = buffer->buffer[index].status;
			if ((status != BUF_STATUS_IDLE) && (status != BUF_STATUS_WRITTEN)) {
				buffer->buffer[index].status = BUF_STATUS_SENT;
				local_irq_restore(flags);
				buffer->trans_index = index;
				if(buffer->save_index == index)
					buffer->save_index = 1- index;
				return index;	
			}
			index = 1 - index;
		}
		local_irq_restore(flags);
	}
	return 0xFF;
}

void pingpang_buffer_send_complete(struct modem_buffer *buffer,int index)
{
	if(buffer->type == BUF_SEND){
		if(buffer->trans_index == index){
			buffer->trans_count += buffer->buffer[index].write_point;
			buffer->buffer[index].write_point = 0;
			buffer->buffer[index].read_point = 0;
			buffer->trans_index = 0xFF;
			buffer->buffer[index].status = BUF_STATUS_IDLE;
			up(&buffer->buf_write_sem);
		}
	}
}
int pingpang_buffer_write(struct modem_buffer *buffer,const char __user *data,int size)
{
	int index = 0;
	int data_size;
	int free_space;
	int write_point;
	int save_flag = 0;
	int i;
	unsigned long flags;
	enum BUF_status_t status;
	
	if(buffer->type == BUF_SEND){
		if (size >= buffer->buffer[0].size)
			return 0;

		do{
			index = buffer->save_index;
			for(i=0;i<2;i++){
				local_irq_save(flags);
				status = buffer->buffer[index].status;
				if ((status != BUF_STATUS_SENT) && 
				    (status != BUF_STATUS_FULL) &&
				    (status != BUF_STATUS_WRITTEN)) {
					buffer->buffer[index].status = BUF_STATUS_WRITTEN;
					local_irq_restore(flags);
					data_size = buffer_data_size(&buffer->buffer[index]);
					free_space = buffer->buffer[index].size -1 - data_size;
					if(free_space < size){
						buffer->buffer[index].status = BUF_STATUS_FULL;
						if(buffer->trans_index == 0xFF)
							buffer->trans_index = index;
						index = 1 - index;
						buffer->save_index = index;
					} else {
						write_point = buffer->buffer[index].write_point;
						copy_from_user(&buffer->buffer[index].addr[write_point],data,size);
						buffer->buffer[index].write_point = write_point + size;
#if 0			
						for(i=0;i<size;i++){
							buffer->buffer[index].addr[write_point++]=data[i];
							if(write_point >= buffer->buffer[index].size)
								write_point = 0;
						}
						dloader_record_timestamp(0x10000000|(index<<24)|buffer->buffer[index].write_point);
						dloader_record_timestamp(0x20000000|(index<<24)|write_point);
						buffer->buffer[index].write_point = write_point;
#endif
						buffer->buffer[index].status = BUF_STATUS_NOEMPTY;
						save_flag = 1;
						
					}
				} else {
					local_irq_restore(flags);
					index = 1 - index;
				}	
			}
			if(save_flag==0)
				down(&buffer->buf_write_sem);
		}while(save_flag == 0);
	}
	return size;
}
int pingpang_buffer_init(struct modem_buffer *buffer,int size)
{
	char *address;
	int  divide = 2;

	address = kzalloc(size + 128, GFP_KERNEL);
	if (address == NULL)
		return -ENOMEM;

	if (buffer->type == BUF_RECV)
		divide = 4;
	
	buffer->buffer[0].addr = address+32;
	buffer->buffer[0].size = size - size/divide;
	buffer->buffer[0].write_point = 0;
	buffer->buffer[0].read_point = 0;
	buffer->buffer[0].status = BUF_STATUS_IDLE;
	buffer->buffer[1].addr = address + buffer->buffer[0].size+64;
	buffer->buffer[1].size = size - buffer->buffer[0].size;
	buffer->buffer[1].read_point = 0;
	buffer->buffer[1].write_point = 0;
	buffer->buffer[1].status = BUF_STATUS_IDLE;
	buffer->lost_count = 0;
	buffer->trans_count = 0;
	buffer->save_index = 0;
	buffer->trans_index = 0xff;
	sema_init(&buffer->buf_read_sem,0);
	sema_init(&buffer->buf_write_sem,0);
	return 0;
}
void pingpang_buffer_free(struct modem_buffer *buffer)
{
	if (buffer->buffer[0].addr){		
		kfree(buffer->buffer[0].addr-32);
		buffer->buffer[0].addr = NULL;
		buffer->buffer[0].size = 0;
		buffer->buffer[0].write_point = 0;
		buffer->buffer[0].read_point = 0;
		buffer->buffer[0].status = 0;
		buffer->buffer[1].addr = NULL;
		buffer->buffer[1].size = 0;
		buffer->buffer[1].read_point = 0;
		buffer->buffer[1].write_point = 0;
		buffer->buffer[1].status = 0;
	}
}
