ittiam code STA/AP ģʽ�޸ļ�¼��



bug fix˵����
===========
2013-02-27
1.staģʽ���޸���wpa/wpa2������ʾΪwep bug��
2.staģʽ���л�power type�������¹���������ʱ�õ�driver ��ps���ԡ�
3.sta/apģʽ������open/close��exit/init�ӿ��еĲ�����

2013-02-20
1.����ps�����رղ����¹�����
2.�������������߲��ԣ�2sһ��ͳ�����ڣ���æ����active״̬�����н���ps��
3.���Խ���ap������A-ASDU��
4.���APģʽ�£�STA���Ϻ��л�ģʽ��STA���Ӳ���AP��AP����Beacon֡���⡣
ע�⣺�˰���Ĭ��֧������802.11n���ܣ�
      (1). A-MSDU�ۺϹ���(�������ͺͽ���A-MSDU�ۺϹ��ܣ�����AP��Ӧ��A-MSDUʱ����Ҫʹ��֧��AMSDU�ۺϹ��ܵ�AP��������AP�п���AMSDU�ۺϹ���).
      (2). ֧��RX Block-Ack��TX Block-Ack����δ������
      (3). ֧�ֽ���AMPDU�ۺϰ�������AMPDU����δ������

2013-02-15:
1.staģʽ������androidӦ�ö�wifi�����Ļ�ȡ�ӿڡ�
2.staģʽ������scan ap�б��������20��ap info��
3.staģʽ��֧��802.1x��֤��ʽ��
4.apģʽ���޸�����������ps��timֵû�и���bug��
5.staģʽ���޸�ui���л�ap���ӣ����֡���ͣ�á���
6.sta/apģʽ���޸���/�ر�ģ��ʱ����֡����ԡ���������bug��
7.staģʽ���Ż���ui�������Ӧ���߼��жϡ�
8.staģʽ�������жϣ���ǰ�շ���æʱ��scan�����ִ�С�
9.staģʽ���޸���wpa/wpa2������ʾΪwep bug��

2013-02-05:
1.����trout2 rf 55 ����bt������Դ��롣
2.����Ż���rx dscrֻ�����״̬λ��δ���dscr��ǰ6Word���ܵ��³�����ķ��գ��Ѿ��Ż�Ϊblock write��ʽ��״̬������ͨ��
block write��ʽ��dscr��ǰ6��word���ֱȲ���ram write��ʽ��������ˣ�Ϊ��������£����ε�block write��ʽ������


2013-02-04:
1.����trout2 fpga rf55 �� rf2829 �Լ� MF EVB�ĺ꣬����rf���á�
2.config.mk�����Ӻ꣬������ݾ����Ӳ���������в��ԡ�

2013-01-31:
1. ����STA�µ�TX Block-Ack���ܣ�����STA�µ�Block-Ack TX/RX������������������TX��block-ack��Ȼ�кܶ�RTS��Ӱ�����¡�
   ���⣬APģʽ�µ�TX Block-Ack���ܻ������⡣
2. �˰��п��Բ���Ӳ����802.11nģʽ�µļ�����Ƿ�������A-MSDU/A-MPDU/Block-Ack��ʹ����ֻ����amsdu_aggr.c�д򿪶�Ӧ
   �ĺ꼴�ɣ�������Ҫ˵�����ǣ�block-ack��A-MPDU�����ʱ����һЩ���⣬��ʱ����ͬʱ������

2013-01-23:
1.�޸�rx����bufferʹ���У�������Ϊ�����ֻ���״̬��û�����ʹ�ü��������º����а���ʱ��û�б���buffer��ʹ�����⡣

2013-01-16:
1. ���ϵͳ����ʱ��mem��ʼ��(rx,tx)ʧ��ʱ�������ƣ���ֹʧ��ʱ������������е����������⡣


2013-01-12: chwg
1. �Ż����޸�reset_mac()�������̣����reset_macʱ����������Ҳ�ڽ��е���ϵͳ�ҵ����⡣

2013-01-12: chenq
1.�޸ĸ�λʱ�¼�ͳ�Ƴ����쳣��������ѭ�����������Ϊwlan��һֱ���ԣ�
2.�������ӳ��Դ�����������sta���ٷ������ӡ��������Ϊui��ʾ��׼ȷ��
3.�޸�scanʱ�����ò�����ÿ���ŵ�ʱ����20ms��Ϊ10ms��
4.��������ʱ��mac��ַƥ�䣬��������ͬ��essid�ĳ��֡�
5.����޸�wpa_supplicant��������mac��ַ������essid���Ż����û����顣

2013-01-10��
1. ȥ��rx�����е������������û�����������rx������б�������ֹreset_macʱ�����߲�ͬ�������쳣�����⡣

2.���������ļ������޸�soft apģʽ�µ�mac��ַ���ŵ���
ʹ�÷�����
*���д���trout_wifi.cfg���ļ����ݸ�ʽ
mac=xx:xx:xx:xx:xx:xx
channel=x

*����
push ��/system/lib/modules/
chmod 777 trout_wifi.cfg

3.staģʽ������һ�������ڵ�AP��������Ϊ�źŲ�����ԭ�򱻹��˵���APʱ��
��Ϊ���ܳɹ����Ӷ�Ӱ���UI����ap�б��޷����µ����⡣

2013-01-9:
1. ��init_mac_mem_pools()�м������жϷ���ֵ����ֹ����ʼ��ʱ������ڴ治�ɹ����³�����������ߵ����쳣���������⡣

2013-01-07:
1.����sta������ʾ����
2.�����ر�wlan����soft ap��UI��ס��
3.����sta��������wpa/wpa2���ܷ�ʽ��ap����Ϊ��ʱ�䲻��NULL frame�������
4.��������soft ap���ܷ�ʽΪWPA2-AES�������ϡ�
5.staģʽ���л�����ap�󣬽�ԭ�����ӵ�ap��ʾ����ͣ�á�����Ϊ���ѱ��桱
6.�޸���staģʽ��ɨ��AP�����г�ʱ�䣨����30�룩�޷�ɨ�赽�������Ҹ��ź�ǿ��AP��BUG��

������apģʽ���ŵ��Լ�mac��ַ�л�
iwconfig wlap0 rate 1��9��mac�����һλ����1��9���ŵ�Ҳ��1��9��

2013-01-06:
1. ��������iperf����ʱ������UIɨ����棬ɨ��APһ��ʱ���Ժ󣬷�����һ������ʾ�豸�Ͽ����ӣ�������ȷ�ϣ����������������link_lose��ֵ̫С���£�
������������Ϊ50�Σ��������޸�Ϊ50�Ρ�

2013-01-05��
1. �޸�BA rx�쳣����ϵͳ�������⣬����λ��Ϊprocess_ba_rx_buff_frame()��wlan_rx.sa/wlan_rx.daΪ�յ��¿�ָ�뵼��������

2012-12-31:
1. AP/STA�¶���������������

2012-12-29��
1. ����հ����������ڹҰ�ʱδ������Ӷ���β������Ӳ��next�Ĵ������³����bug.

2012-12-29:
1. ����հ������������հ��жϴ���rx_complete_isr()��rx_complete_work()��ͬ������ͬһ��dscr��shedule�������ε���trout�հ����д������⡣

2012-12-24:
1. APģʽ�£�STA����AP����block-ack����Э���У���ʱ��ΪSTA��ĳ��ԭ��Ͽ���AP��֪����Ȼ��STA���½���block-ackЭ����֤��
   AP��ΪSTA�Ѿ��������������̲���û����addba�����������̣���˶�STA������block-ackЭ�̲��������Ӧ��
   
   ���������AP�ڽ��յ�block-ack����֮�����Ȳ鿴�Ƿ��Ѿ����STA������block-ackЭ����Ϣ������У���������STA��ص�block-ack��Ϣ��
   Ȼ���ٽ���block-ack��Ӧ��
   
   �����޸�λ�ã��ļ�management_11n.c�к���handle_wlan_addba_req()��1590�С�
-------------------------------------------------------------------------------------------------------------
2012-12-21:
1. �޸Ľű���������ԡ�

2012-12-20:
1.���߽���2.2.2�汾�ϼ�Ϊ2.3.5�汾��
��config.mk�м���꿪��TROUT_WIFI_FPGA
����fpga������򿪴˺ꡣ

2012-12-19:
1. �ϲ����߹���Android2.2.2�Ͳ���Android2.3.5���������bug�İ汾�ͳ��ĸ��Ż���İ汾��
2. ��������STA��APģʽ����������������

2012-12-17:
1. �޸���rx������do_rx_preprocess()�г��������Ƶ����⡣
2. �˰�Ϊ�ϲ������޸ĵİ汾֮ǰ��trout�Ż��汾���°汾���ݡ�

2012-12-11:
1. �ϲ��������tx����tx share memory���ֳ����飬��ʱ���ã���߷���Ч�ʡ�
2. ֮ǰ��ӵ�trout tx/rx buffer��̬����Ӧ������ʱδ�򿪣�׼�������޸ġ�

2012-12-10:
1. ������trout tx/rx buffer��̬����Ӧ���ơ�
2. �����sta��802.11n onlyģʽ���հ�����ʱ������Ϊ�����������ϵͳ������bug������ԭ��Ϊ����11nģʽ�£�process_all_event��⵽�д�����
   �¼�����ʵ��ȥ����ʱ��ȴ��Ϊevent��alloc����û��post�����Ե���buffer״̬Ϊ2��event��״̬Ϊ1��event��ϣ�get_eventʼ�ջ�ȡ����������
   ��process_all_event()��while()��һֱѭ����������ʱ��ͬʱ�޸�send_data_frame_to_host_if()�в���Ҫ��post event��ʽ��������Ҫע�⣺
   process_all_event()��g_event_cnt�Ĵ��������ڳ����������ط���������process_event_queues()�еĺ���������g_event_cnt�Զ���һ���п�����
   �˴�Ԥ��Ĳ�һ�¡�
3. ��802.11n onlyģʽ�£�Ƶ����ӡsend delBA�����׷��ԭ����������nģʽ�£��յ���AMPDU�ۺϰ������ۺϰ��в���802.11n��ͷ�����Ǵ����л���
   ����802.11 MACͷȥ���������½�������MAC��ַ��entry�����Ҳ����������Ϊ��MAC��ַû��Э���Ƿ����Block-Ack����Ϳ�ʼ�˿鴫�䣬��˷���
   �����鴫������(��δ�������Ҫ���۴���ʽ����ǰ��ʱ���ε�)��

2012-11-29:
1.  �������essid����adb shellû����Ӧ�����Ѿ�û���ˡ�

2012-11-26:
1. ȥ��tx_pkt_process����ʱ���޸�trout��tx_dscr״̬�Լ��ж�is_trout_tx_mem_empty()�ж�tx dscr��״̬������IO������
2. ȥ��qmu_cpy_to_trout���������͹����м��tx pointer��lock_bit�Ƿ�ռ�ã�����IO�������޸�û�г������⡣

2012-11-21:
1. ��mac_isr_work�����normal-rx, high-rx, tx�жϷֱ𵥶����жϷ�ʽ��Ϊһ�������
2. ��mac_isr_work��ÿ�ζ�int mask reg��ȡint maskֵ��Ϊͨ��ȫ�ֱ���g_int_mask����ȡ��
3. ��qmu_cpy_to_trout�ж�TSF reg��Ϊ��һ�εĳ�ֵ��tsf reg��ȡ��������ֵͨ����ֵ���ϱ���ʱ���ۻ�ֵ�����£�һ��ʱ����(10s)֮��
   �ٴζ�tsf reg��ֵ��Ϊ��ֵ�Խ���tsfֵ������IO������
2012-11-10:
1. ȥ��Э��ջ����mac_xmit()ʱ������txq full������Ч��pkt memory copy.
2. ȥ��spi_interface.c�ж�дreg/ram�ӿڶ���Ļ�������
3. Ϊ�����tx���£���trout tx share memory size��8KB����32KB��rx��Ӧ�ļ�С��

2012-11-02��
1. ���ڷ���ʱ������tx descr��TSF�����update_tx_dscr_tsf_ts()�ܺ�ʱ������80us�ļ��ʱȽϴ����Ծ���������üĴ�����
   �޸ķ�ʽΪ��������һ��DMA����qmu_cpy_to_trout()ʱ��ֻ��һ��tsf register��Ȼ����������tx descr��ֵ���ô�ֵ������
   ��������⡣

2012-10-30��
1. ���������жϲ�������I/O������ͨ���жϷ�ʽ�������������ڶ��ж����⡣

2012-10-23:
�Ż���¼��
1. ��tx_pkt_process()��I/O��ʽ�ض�tx dscr��Ϣ��Ϊͨ��DMA��ʽ��ȡ���Ӷ�����I/O����������
2. ��ԭ��qmu_cpy_to_trout()�зֶ�ΰ������ݵ�trout�ķ�ʽ�Ļ���������Ƚ���ΰ��Ƶ����ݰ��Ƶ�һ����buffer�У�Ȼ����ͨ��1��DMA����
   ��trout�У�������DMA����С�����ݵĴ���(ͨ��config.mk�к�TX_PKT_USE_DMA����)��
3. ��ǰһ��qmu_cpy_to_trout�������Ƿ�Ӳ���������ͳɹ��ж���ԭ�����ж�ÿ��tx dscr״̬��Ϊֻ�ж����һ��tx dscr��������I/O������
4. ��mac_xmit������qmu_cpy_to_trout��Ϊֻ��txq_handle��tx dscr <= 1ʱ��ͨ��mac_xmit������ʽ��������qmu_cpy_to_troutȫ����tx isr
   �������������ͱ����˲�ѯtxq�Ƿ�Ϊnull������Ķ��I/O��������tx���ڶ��ж�������ˣ��˷�������ʽ�����в����С�

2012-10-12:
1. ��is_all_machw_q_null()�������ø�Ϊÿ��5�ζ�ȡһ���Ա㽵�Ͷ�I/O�������������ڲ���(��õķ�������event�����е��ô˺���ʱ���ж�ÿ��5�ε���һ�Σ���������
Ӱ�쵽tx isr�е��ô˺���)��
2. �Ż�tx_pkt_process()����ͨ��DMA�ķ�ʽ����I/O������

2012-10-10:
1. ��trace�������irq��event_wq��I/O��������ͳ�Ƶ���Ϣ�����㶨λCPUռ��ͳ�ơ�
ͨ����adb shell�����룺
echo print_count_info > /sys/devices/platform/sprd-sdhci.1/mmc_host/mmc1/mmc1:0001/mmc1:0001:1/trout;dmesg -c
���ɲ鿴ͳ����Ϣ��
���⻹����ͨ����
echo clean_count_info > /sys/devices/platform/sprd-sdhci.1/mmc_host/mmc1/mmc1:0001/mmc1:0001:1/trout;dmesg -c
���ͳ����Ϣ��
2012-10-08:
1. ��trace�Ļ����Ϻϲ���ˮ�����°�Ĺ̼���udp���������У�802.11bgnģʽ��δ���ַ�������ֻ�м���KB���⣬��
receive_from_host()��event����ʧ��������Ȼ���ڡ�
2. ����STA/APģʽ�£�spi write ramʱ��Beacon֡ǰû��Ԥ��8bytes��Bug��

2012-09-29:
1. trace�ӿ��Ѿ��Ӻã�ͨ��adv_config.mk�к�"TROUT_TRACE_DBG"�����أ��������ں�"DEBUG_MODE"��
���⣬�����з���trout udp����ʱ��gģʽ�·�����������802.11bgnģʽʱ����������ֻ�м���KB������
����receive_from_host()��event����ʧ�ܵ��£�����ˮ�����������汾ȴû�д����⣬����Ա�ȷ�ϡ�

2012-09-04:
1. ����ittiam_mac_v1.3�档

2012-08-23
modify wifi direct (P2P) feature
1. P2P����ʹ�õ���share ramβ���Ŀռ䣬����������������ֿռ����������£�����rx buf����32�����У���
   ������share ramʹ�ÿռ䳬����Χ��
2. ��config.mk�ļ���: �ѹص�DEFAULT_SME�ꣻ�Ѵ�MAC_P2P�꣬ʹ��P2P���ܡ�
3. ����P2P���ܵ���ϸ�ĵ���svn�ϵĵ�ַ��http://10.6.2.16/svn/trout/6820/6820_doc/wifi_direct/wifi_direct_readme.txt

2012-08-06:
1. �Ż�������trout rx���ִ��룬�������ⲻ��Ҫ�Ķ�trout���ϵͳ��Դ���ġ�

2012-08-03:
1. ������Counter�Ĵ�����
//ACK/CTS/RTS/FCS fail/etc(detail in Received Frame Filter Register-0x80) Filter Counter when RX Frame Done
#define rMAC_RX_FRAME_FILTER_COUNTER        (PA_BASE + 0x0050)

//Address etc filter counter(detail in Received Frame Filter Register) after RX MAC Header done
#define rMAC_RX_MAC_HEADER_FILTER_COUNTER   (PA_BASE + 0x0054)

//the counter for the losted frames when the Rx Q is Full.
#define rMAC_RXQ_FULL_COUNTER          (PA_BASE + 0x005C)

//the counter for the rx frames into the rx q.
#define rMAC_RX_RAM_PACKET_COUNTER          (PA_BASE + 0x00C4)

2. ������Ƚ�ཨ�飬�޸�һЩ�Ĵ���������  
       			     PHY Bank 1��  address      	data
						0x70		0x29
						0x71		0x30
				9861:		address 	data
						0xd 		0x80
						0x10 		0x80
						0x11 		0xff
	����ȥ����DC offset֮�󷢰�����ܺã�CRC error����û��

3. ������iwconfig iwlan0 rate auto������õ��Խӿڡ�
4�������Tx�����޷��ѻ���bug��


2012-08-01:
1. ������trout tx���ִ��룬�������򷢰��ж���num_dscr��׼ȷ�����ķ���������ȥ�����⣬����δ��ȫ�����


2012-07-23��
1. �ϲ��˳������µ����ó��򣬽������"validate_buffer"���µ�ϵͳReset����
2. ��������ͬ0721�棬��������
3. spi�ӿ��У�ʹ����DMA��spiʱ�Ӹ�Ϊ24MHZ����Ҫ����boot_imageΪboot_cs24_gap11.img��

2012-07-21��
1. APģʽ�£��޸��˶��sta������Ap֮�󣬻�ping����������ͨ�����⡣
2. �ϲ���SDIO�ӿڡ�

2012-07-20:
1. STA/APģʽ�£��޸���ϵͳ������bug��

2012-07-18��
1. AP/STAģʽ��������������
2. AP/STAģʽ��֧��802.11b/g/nģʽ������nģʽ�£���֧��A-MSDU�ۺϹ��ܣ���ping������
3. �޸��˲��ֵ����ں�Oop��invalid_buffer����ϵͳreset�����⣬�����ϵͳ�ȶ��ԡ�

2012-07-16��
1. APģʽ�£��ڻ���7.15�汾�ϣ�����RF��������ӦFPGA�汾��trout_fpage_spi_u2_syn_0710_ada.bin;
��Ҫ����������
(1) TX/RXʱ�ӣ�
   system reg 0x62��WORD��ַ��������Ϊ0x800000, ��BIT23����Ϊ1
   system reg 0x63��WORD��ַ��������Ϊ0x20, ��BIT5����Ϊ1
��Ӧ����λ�ã�csl_linux.c: line296~299;
(2) �޸�9863������:
	MFE_TX7_VAL 	0xFF �޸�Ϊ0x60
    MFE_IOC1_VLAN	0xC0 �޸�Ϊ0x00
��Ӧ����λ�ã�phy_ittiam.h: line583, line540;
(3) ʹ�ܺ���write_mxfe_reg():
��Ӧ����λ�ã�phy_ittiam.h: line620~640;    
(4) �޸�phy_max2829_ittiam.h������
#define TXPOWER11A    0x3F
#define TXPOWER11B    0x3F
��Ӧ����λ�ã�phy_max2829_ittiam.h: line429~430;	


2012-07-15:
1. APģʽ��802.11b/g/nģʽ�£�֧��nģʽ�£�A-MSDU�ۺϹ���(��������֤FPGAоƬ����)��

2012-07-12:
1. APģʽ��802.11b/g/nģʽ�£�����pingͨ�����ݰ�(����1518)���������ִ��뼰bug��
2. �ϲ�AP/STAģʽ���룬��STAģʽ�»�δ��֤��

2012-07-04:
1. ʵ����reset_phy_ittiam()�Ĺ��ܣ�������spi��д�Ĵ����ӿ�(��Ҫ�޸��˺�SPI_RD_ERG_CMD��SPI_WR_ERG_CMD)��
2. ��ʵ��������ͬһ��AP�µĶ�̨PC����pingͨ���ܡ�