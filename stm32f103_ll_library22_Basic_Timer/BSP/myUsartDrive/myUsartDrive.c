#include "myUsartDrive/myUsartDrive.h"
#include "usart.h"

volatile uint8_t rx_buffer[RX_BUFFER_SIZE]; // ����DMAר�û�����
uint64_t g_Usart1_RXCount = 0;              // ͳ�ƽ��յ��ֽ���

uint8_t tx_buffer[TX_BUFFER_SIZE];          // ����DMAר�û�����
volatile uint8_t tx_dma_busy = 0;           // DMA���ͱ�־λ��1�����ڷ��ͣ�0�����У�

volatile uint16_t usart1Error = 0;
volatile uint16_t dma1Channel4Error = 0;
volatile uint16_t dma1Channel5Error = 0;

/* usart1����ringbuffer */
volatile lwrb_t g_Usart1RxRBHandler; // ʵ����ringbuffer
volatile uint8_t g_Usart1RxRB[RX_BUFFER_SIZE] = {0,}; // usart1����ringbuffer������

/**
  * @brief  ����DMA1ͨ��5����USART1_RX���ա�
  * @note   �˺�������LL��ʵ�֡�����DMA1ͨ��5���ڴ��ַ����Ϊrx_buffer��
  *         �����ַ����ΪUSART1���ݼĴ����ĵ�ַ�������ô������ݳ���ΪRX_BUFFER_SIZE��
  *         ���ʹ��DMA1ͨ��5�������������ݡ�
  * @retval None
  */
static void DMA1_Channel5_Configure(void) {
    /* ����DMA1ͨ��5����USART1_RX���� */
    LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_5, (uint32_t)rx_buffer);
    LL_DMA_SetPeriphAddress(DMA1, LL_DMA_CHANNEL_5, (uint32_t)&USART1->DR);
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_5, RX_BUFFER_SIZE);
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_5);
}

/**
  * @brief  ʹ��DMA�����ַ���������USART1_TX��Ӧ��DMA1ͨ��4
  * @param  data: ����������ָ�루����ָ��������ͻ�������
  * @param  len:  ���������ݳ���
  * @retval None
  */
void USART1_SendString_DMA(const char *data, uint16_t len)
{
    if (len == 0 || len > TX_BUFFER_SIZE) { // ������DMA����0���ֽڣ��ᵼ��û�취���뷢������жϣ�Ȼ�������������
        return;
    }
    // �ȴ���һ��DMA�������
    while(tx_dma_busy);
    tx_dma_busy = 1; // ���DMA���ڷ���
    memcpy(tx_buffer, data, len); // ����Ҫ���͵����ݷ���DMA���ͻ�����
    // ���DMAͨ��4����ʹ�ܣ����Ƚ����Ա���������
    if (LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_4)) {
        LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_4);
        while(LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_4));
    }
    // ����DMAͨ��4���ڴ��ַ�������ַ�����ݴ��䳤��
    LL_DMA_SetMemoryAddress(DMA1, LL_DMA_CHANNEL_4, (uint32_t)tx_buffer);
    LL_DMA_SetPeriphAddress(DMA1, LL_DMA_CHANNEL_4, (uint32_t)&USART1->DR);
    LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_4, len);
    // ����USART1��DMA��������CR3��DMAT��1��ͨ��Ϊ��7λ��
    LL_USART_EnableDMAReq_TX(USART1); // ����USART1��DMA��������
    // ����DMAͨ��4����ʼDMA����
    LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_4);
}

/**
  * @brief  ����⵽USART1�쳣ʱ�����³�ʼ��USART1�������DMAͨ����
  * @note   �˺�������ֹͣDMA1ͨ��4��USART1_TX����ͨ��5��USART1_RX����ȷ��DMA������ֹ��
  *         Ȼ�����USART1����DMA�������󣬲�ͨ����SR��DR�������Ĵ����־��
  *         ����������ʱ�����MX_USART1_UART_Init()��������USART1��GPIO��DMA��
  *         MX_USART1_UART_Init()�ڲ������USART1�жϵ����ã��������ʹ�ܡ�
  * @retval None
  */
void USART1_Reinit(void)
{
    /* ����DMA�����USART1������DMA�Ļ� */
    LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_4); // ����DMA1ͨ��4
    while(LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_4)) { // �ȴ�ͨ����ȫ�رգ�������ӳ�ʱ�����Ա�����ѭ����
        // ��ѭ���ȴ�
    }
    LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_5); // ����DMA1ͨ��5
    while(LL_DMA_IsEnabledChannel(DMA1, LL_DMA_CHANNEL_5)) {  // �ȴ�ͨ����ȫ�رգ�������ӳ�ʱ�����Ա�����ѭ����
        // ��ѭ���ȴ�
    }
    
    /* USART1 */
    NVIC_DisableIRQ(USART1_IRQn); // 1. ����USART1ȫ���жϣ��������������в����µ��жϸ���
    LL_USART_DisableDMAReq_RX(USART1); // 2. ����USART1��DMA�����������ʹ��DMA���գ�
    LL_USART_Disable(USART1); // 3. ����USART1
    
    // 4. ��SR��DR���������Ĵ����־������IDLE��ORE��NE��FE��PE��
    volatile uint32_t tmp = USART1->SR;
    tmp = USART1->DR;
    (void)tmp;
    
    for (volatile uint32_t i = 0; i < 1000; i++); // 5. ��ѡ��������ʱ��ȷ��USART��ȫ�ر�
    tx_dma_busy = 0; // ��λ���ͱ�־
    MX_USART1_UART_Init(); // ���³�ʼ��USART1
    DMA1_Channel5_Configure(); // ���³�ʼ��DMA1ͨ��5
}

/**
  * @brief  ���DMA1ͨ��4������󣬲��������״̬��
  * @note   �˺�������LL��ʵ�֡���Ҫ���ڼ��USART1_TX���������DMA1ͨ��4�Ƿ����������TE����
  *         �����⵽��������������־������DMA1ͨ��4�����ر�USART1��DMA��������
  *         �Ӷ���ֹ��ǰ���䡣����1��ʾ�����Ѵ������򷵻�0��
  * @retval uint8_t ����״̬��1��ʾ��⵽�������˴���0��ʾ�޴���
  */
__STATIC_INLINE uint8_t DMA1_Channel4_Error_Handler(void) {
    // ���ͨ��4�Ƿ����������TE��
    if (LL_DMA_IsActiveFlag_TE4(DMA1)) {
        // �����������־
        LL_DMA_ClearFlag_TE4(DMA1);
        // ����DMAͨ��4��ֹͣ��ǰ����
        LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_4);
        // ���USART1��DMA��������DMATλ��
        LL_USART_DisableDMAReq_TX(USART1);
        // ������ͱ�־����
        tx_dma_busy = 0;
        return 1;
    } else {
        return 0;
    }
}

/**
 * @brief  USART1����DMA1_Channel4�жϴ�����
 * 
 * ����USART1ͨ��DMA1ͨ��4���͹����в������жϣ�������
 * - DMA�������
 * - ������ɣ�TC��Transfer Complete��
 * 
 * @note
 * - ʹ��LL�����DMA��USART�Ĵ���λ��
 * - ������ɺ�ر�DMA����ͨ���������USART��DMAT����λ��
 * - ͨ����־����tx_dma_busyָʾ������ɣ�������һ�η��͡�
 * 
 * @details
 * 1. ���DMA�������ͳ�ƴ��������
 * 2. ��鷢������жϣ�TC����
 *    - ���TC�жϱ�־��
 *    - ����DMA1_Channel4����ֹ�󴥷���
 *    - ��ֹUSART1��DMA��������DMATλ���㣩��
 *    - �������æ��־(tx_dma_busy = 0)��
 * 
 * @warning
 * - �жϴ�������У����뼰ʱ���TC�жϱ�־�������жϻ����������
 * - ����DMA��������ǰ��ȷ��DMA�����Ѿ���ɡ�
 */
void USART1_TX_DMA1_Channel4_Interrupt_Handler(void)
{
    if (DMA1_Channel4_Error_Handler()) { // ��ش������
        dma1Channel4Error++;
    } else if(LL_DMA_IsActiveFlag_TC4(DMA1)) {// ��鴫����ɱ�־��TC���Ƿ���λ��LL���ṩTC4�ӿڣ�
        // ���DMA������ɱ�־
        LL_DMA_ClearFlag_TC4(DMA1);
        // ����DMAͨ��4��ȷ���´δ���ǰ��������
        LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_4);
        // ���USART1��DMATλ���ر�DMA��������
        LL_USART_DisableDMAReq_TX(USART1);
        // ���DMA�������
        tx_dma_busy = 0;
    }
}

/**
  * @brief  ���DMA1ͨ��5������󣬲��ָ�DMA���ա�
  * @note   �˺�������LL��ʵ�֡���Ҫ���ڼ��USART1_RX���������DMA1ͨ��5�Ƿ����������TE����
  *         �����⵽��������������־������DMA1ͨ��5�����ô��䳤��ΪRX_BUFFER_SIZE��
  *         ������ʹ��DMAͨ��5�Իָ����ݽ��ա�����1��ʾ�����Ѵ������򷵻�0��
  * @retval uint8_t ����״̬��1��ʾ��⵽�������˴���0��ʾ�޴���
  */
__STATIC_INLINE uint8_t DMA1_Channel5_Error_Hanlder(void) {
    // ���ͨ��5�Ƿ����������TE��
    if (LL_DMA_IsActiveFlag_TE5(DMA1)) {
        // �����������־
        LL_DMA_ClearFlag_TE5(DMA1);
        // ����DMAͨ��5��ֹͣ��ǰ����
        LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_5);
        // �������ô��䳤�ȣ��ָ�����ʼ״̬
        LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_5, RX_BUFFER_SIZE);
        // ����ʹ��DMAͨ��5���ָ�����
        LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_5);
        return 1;
    } else {
        return 0;
    }
}

/**
 * @brief  USART1����DMA1_Channel5�жϴ�����
 * 
 * ����USART1ͨ��DMA1ͨ��5���չ����в������жϣ�������
 * - DMA�������
 * - �봫����ɣ�HT��Half Transfer��
 * - ȫ������ɣ�TC��Transfer Complete��
 * 
 * @note
 * - ʹ��LL�����DMA�жϱ�־��ȷ����Ч�����ж�Դ��
 * - �봫���жϱ�ʾǰ����(0~511�ֽ�)������ɣ�
 * - ����жϱ�ʾ�����(512~1023�ֽ�)������ɣ�
 * - ��������ͨ��memcpy���������ͻ���tx_buffer������λrx_complete��
 * 
 * @details
 * 1. ���DMA�������ͳ�ƴ��������
 * 2. �봫���жϣ�
 *    - ���HT��־��
 *    - ���ƽ��ջ���ǰ�벿�����ݡ�
 *    - ���ý������ݳ�������ɱ�־��
 * 3. ����жϣ�
 *    - ���TC��־��
 *    - ���ƽ��ջ����벿�����ݡ�
 *    - ���ý������ݳ�������ɱ�־��
 * 
 * @warning
 * - �жϷ���������ؼ�ʱ���HT/TC�жϱ�־�������жϻ����������
 * - ע��ͬ��DMA���ջ���(rx_buffer)���û�����(tx_buffer)������һ���ԡ�
 */
void USART1_RX_DMA1_Channel5_Interrupt_Handler(void)
{
    if (DMA1_Channel5_Error_Hanlder()) { // ��ش���ʧ��
        dma1Channel5Error++;
    } else if(LL_DMA_IsActiveFlag_HT5(DMA1)) { // �ж��Ƿ�����봫���жϣ�ǰ������ɣ�
        // ����봫���־
        LL_DMA_ClearFlag_HT5(DMA1);
        // ����ǰ 512 �ֽ����ݣ�ƫ�� 0~511��
        lwrb_write((lwrb_t*)&g_Usart1RxRBHandler, (uint8_t*)rx_buffer, RX_BUFFER_SIZE/2); // д��ringbuffer
        g_Usart1_RXCount += RX_BUFFER_SIZE/2;
    } else if(LL_DMA_IsActiveFlag_TC5(DMA1)) { // �ж��Ƿ������������жϣ��������ɣ�
        // ���������ɱ�־
        LL_DMA_ClearFlag_TC5(DMA1);
        // ����� 512 �ֽ����ݣ�ƫ�� 512~1023��
        lwrb_write((lwrb_t*)&g_Usart1RxRBHandler, (uint8_t*)(rx_buffer + RX_BUFFER_SIZE/2), RX_BUFFER_SIZE/2); // д��ringbuffer
        g_Usart1_RXCount += RX_BUFFER_SIZE/2;
    }
}

/**
  * @brief  ���USART1�����־���������ش����־��ORE��NE��FE��PE����
  * @note   �˺�������LL��ʵ�֡�����⵽USART1�����־ʱ��ͨ����ȡSR��DR�������
  *         ������1��ʾ���ڴ��󣻷��򷵻�0��
  * @retval uint8_t ����״̬��1��ʾ��⵽�����������0��ʾ�޴���
  */
__STATIC_INLINE uint8_t USART1_Error_Handler(void) {
    // ���������USART�����־��ORE��NE��FE��PE��
    if (LL_USART_IsActiveFlag_ORE(USART1) ||
        LL_USART_IsActiveFlag_NE(USART1)  ||
        LL_USART_IsActiveFlag_FE(USART1)  ||
        LL_USART_IsActiveFlag_PE(USART1))
    {
        // ͨ����SR��DR����������־
        volatile uint32_t tmp = USART1->SR;
        tmp = USART1->DR;
        (void)tmp;
        return 1;
    } else {
        return 0;
    }
}

uint8_t USART1_Put_Data_Into_Ringbuffer(const void* data, uint16_t len)
{
    uint8_t ret = 0;
    if (len >= RX_BUFFER_SIZE) {
        if (len > RX_BUFFER_SIZE) {
            len = RX_BUFFER_SIZE;
            ret = 1;  // ���ݳ��ȴ���ringbuffer�ܿռ䣬��������ݱ�����
        }
        lwrb_reset((lwrb_t*)&g_Usart1RxRBHandler); // ����ringbuffer
        lwrb_write((lwrb_t*)&g_Usart1RxRBHandler, data, len); // �����ݷ���ringbuffer
    } else {
        lwrb_sz_t freeSpace = lwrb_get_free((lwrb_t*)&g_Usart1RxRBHandler);
        
        if (freeSpace > len) { // ringbuffer�ж�����з�������
            lwrb_write((lwrb_t*)&g_Usart1RxRBHandler, data, len);
        } else {
            lwrb_skip((lwrb_t*)&g_Usart1RxRBHandler, len - freeSpace); // ֻskip��Ҫ�Ŀռ�
            lwrb_write((lwrb_t*)&g_Usart1RxRBHandler, data, len); // �����ݷ���ringbuffer
            ret = 2; // ringbuffer�ľ����ݱ�����������������
        }
    }
    return ret;
}



/**
 * @brief  USART1�жϴ�����
 * 
 * ����USART1�������жϣ�������
 * - �����⣨֡�������������������ȣ�
 * - ���տ����жϣ�IDLE�����������ݰ�����DMAͨ����������
 * 
 * @note
 * - ʹ��LL��ֱ�Ӳ���USART�Ĵ�����DMA�Ĵ�����ȷ����Ч����
 * - �����ж����ڿ����ж�һ֡������ɣ�����ȴ�������������
 * - Ϊ����DMA�봫���жϻ�����ж�ʱ�����ظ������������ʣ���������жϵ�ǰ֡���ݡ�
 * 
 * @details
 * 1. ��鲢����USARTӲ���������У���
 * 2. ���USART�����жϣ�IDLE����
 *    - ���IDLE��־����SR����DR����
 *    - ����DMA����ͨ������ֹ���ݼ���д�롣
 *    - ����DMAʣ�������ж��ѽ��յ��������������Ƶ����ͻ���tx_buffer��
 *    - ���ý��յ������ݳ���(recved_length)������λrx_complete��
 *    - ����DMA���䳤�ȣ���������DMA���ա�
 * 
 * @warning
 * - ������ȷ���IDLE�жϱ�־������USART�жϽ�����������
 * - ע��DMA���ջ���߽��������������ݴ�λ�򸲸ǡ�
 */
void USART1_RX_Interrupt_Handler(void)
{
    if (USART1_Error_Handler()) { // �������־
        usart1Error++; // �д��󣬼�¼�¼�
    } else if (LL_USART_IsActiveFlag_IDLE(USART1)) {  // ��� USART1 �Ƿ�����ж��ж�
        uint32_t tmp;
        /* ���IDLE��־�������ȶ�SR���ٶ�DR */
        tmp = USART1->SR;
        tmp = USART1->DR;
        (void)tmp;
        // ���� DMA1 ͨ��5����ֹ���ݼ���д��
        LL_DMA_DisableChannel(DMA1, LL_DMA_CHANNEL_5);
        uint16_t remaining = LL_DMA_GetDataLength(DMA1, LL_DMA_CHANNEL_5); // ��ȡʣ�������
        uint16_t count = 0;
        // ����ʣ���ֽ��жϵ�ǰ�����ĸ�����
        // ���У����⵱���ݳ��ȸպ�512�ֽ���1024�ֽ�ʱ����������ж�������жϸ����������ݣ��봫������ж�������жϸ����������ݡ�
        if (remaining > (RX_BUFFER_SIZE/2)) {
            // ���ڽ���ǰ���������������� = (1K - remaining)�����϶����� 512 �ֽ�
            count = RX_BUFFER_SIZE - remaining;
            if (count != 0) { // �����봫������жϳ�ͻ���ิ��һ��
                g_Usart1_RXCount += count;
                lwrb_write((lwrb_t*)&g_Usart1RxRBHandler, (uint8_t*)rx_buffer, count); // д��ringbuffer
            }
        } else {
            // ǰ������д������ǰ�ں��������������������� = (RX_BUFFER_SIZE/2 - remaining)
            count = (RX_BUFFER_SIZE/2) - remaining;
            if (count != 0) { // �����봫������жϳ�ͻ���ิ��һ��
                g_Usart1_RXCount += count;
                lwrb_write((lwrb_t*)&g_Usart1RxRBHandler, (uint8_t*)rx_buffer + RX_BUFFER_SIZE/2, count); // д��ringbuffer
            }
        }

        // �������� DMA ���䳤�Ȳ�ʹ�� DMA
        LL_DMA_SetDataLength(DMA1, LL_DMA_CHANNEL_5, RX_BUFFER_SIZE);
        LL_DMA_EnableChannel(DMA1, LL_DMA_CHANNEL_5);
    }
}

/**
 * @brief  USART1ģ����������
 * 
 * ����ѭ���е��ñ���������ringbuffer������ʱ�����Դ���
 * 
 * @note
 */
void USART1_Module_Run(void)
{
    if (lwrb_get_full((lwrb_t*)&g_Usart1RxRBHandler)) {
        
    }
}

/**
 * @brief  USART1��DMAģ������
 * 
 * ����USART1�����DMAͨ����ʵ�ָ�Ч�Ľ��պͷ��͹��ܡ�
 * ����ʹ��USART��DMA���󡢿����жϣ��Լ�DMA�Ĵ����жϡ�
 * ͬʱ����DMA1_Channel5_Configure()����DMA����ͨ����ʼ�����á�
 * 
 * @note
 * - LL_USART_EnableDMAReq_RX: ����USART1��������ʱͨ��DMA���ˣ�����CPU������
 * - LL_USART_EnableIT_IDLE: ��USART1���տ����жϣ�����֡�߽��⡣
 * - LL_DMA_EnableIT_HT: ����DMA�봫���жϣ�����ǰ�������ݣ���
 * - LL_DMA_EnableIT_TC: ����DMA��������жϣ������������ݣ���
 * - �����ⲿ����DMA1_Channel5_Configure()���н���ͨ���������á�
 */
void USART1_Config(void)
{
    lwrb_init((lwrb_t*)&g_Usart1RxRBHandler, (uint8_t*)g_Usart1RxRB, sizeof(g_Usart1RxRB) + 1); // RX ringbuffer��ʼ��
    
    LL_USART_EnableDMAReq_RX(USART1); // ʹ��USART1_RX��DMA����
    LL_USART_EnableIT_IDLE(USART1);   // ����USART1���տ����ж�
  
    LL_DMA_EnableIT_HT(DMA1, LL_DMA_CHANNEL_5); // ʹ��DMA1ͨ��5�Ĵ�������ж�
    LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_5); // ʹ��DMA1ͨ��5�Ĵ�������ж�
  
    LL_DMA_EnableIT_TC(DMA1, LL_DMA_CHANNEL_4); // ʹ��DMA1ͨ��4�Ĵ�������ж�
    
    DMA1_Channel5_Configure(); 
}
