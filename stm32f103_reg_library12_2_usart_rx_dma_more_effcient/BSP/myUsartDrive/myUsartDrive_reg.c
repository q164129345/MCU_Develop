#include "myUsartDrive/myUsartDrive_reg.h"

volatile uint8_t rx_buffer[RX_BUFFER_SIZE]; // ����DMAר�û�����
uint64_t g_Usart1_RXCount = 0;              // ͳ�ƽ��յ��ֽ���
volatile uint16_t g_DMARxLastPos = 0;       // ��¼��һ�ζ�ȡ��Ϣ�ĵ�ַλ��

volatile uint8_t tx_buffer[TX_BUFFER_SIZE]; // ����DMAר�û�����
volatile uint8_t tx_dma_busy = 0;           // DMA���ͱ�־λ��1�����ڷ��ͣ�0�����У�
uint64_t g_Usart1_TXCount = 0;              // ͳ�Ʒ��͵��ֽ���

volatile uint16_t usart1Error = 0;
volatile uint16_t dma1Channel4Error = 0;
volatile uint16_t dma1Channel5Error = 0;

/* usart1 ringbuffer */
volatile lwrb_t g_Usart1RxRBHandler; // ʵ����ringbuffer
volatile uint8_t g_Usart1RxRB[RX_BUFFER_SIZE] = {0,}; // usart1����ringbuffer������

volatile lwrb_t g_Usart1TxRBHandler; // ʵ����ringbuffer
volatile uint8_t g_Usart1TxRB[TX_BUFFER_SIZE] = {0,}; // usart1����ringbuffer������

/**
  * @brief   ��ȡ USART1 TX DMA ����æ״̬
  * @note    ͨ����ȡȫ�ֱ�־ tx_dma_busy �жϵ�ǰ USART1 �� DMA ����ͨ���Ƿ����ڹ���
  * @retval  0 ��ʾ DMA ���Ϳ��У��ɷ����µĴ���
  * @retval  1 ��ʾ DMA ����æ����ȴ���ǰ�������
  */
__STATIC_INLINE uint8_t USART1_Get_TX_DMA_Busy(void)
{
    return tx_dma_busy;
}

/**
  * @brief  ������ʽ������ NUL ��β���ַ���
  * @param  str ָ���� '\0' ��β�Ĵ������ַ���������
  * @note   ������ͨ����ѯ TXE ��־λ��USART_SR.TXE��λ7�����жϷ������ݼĴ����Ƿ�գ�
  *         - �� TXE = 1 ʱ����ʾ DR �Ĵ����ѿգ���д����һ���ֽ�  
  *         - ͨ���� DR �Ĵ���д�����ݣ�LL_USART_TransmitData8����������  
  *         - �ظ��������̣�ֱ�������ַ��������� '\0'  
  * @retval None
  */
void USART1_SendString_Blocking(const char* str)
{
    /* �Ĵ�����ʽ */
    while(*str) {
        while(!(USART1->SR & (0x01UL << 7UL))); // �ȴ�TXE = 1
        USART1->DR = *str++;
    }
}

/**
  * @brief  ������д��USART1����ringbuffer�С�

  * @param[in] data ָ��Ҫд������ݻ�����
  * @param[in] len  Ҫд������ݳ��ȣ���λ���ֽڣ�
  * 
  * @retval 0 ���ݳɹ�д�룬�����ݶ���
  * @retval 1 ringbufferʣ��ռ䲻�㣬�����ݱ�����������������
  * @retval 2 ���ݳ��ȳ���ringbuffer��������ȫ�������ݱ���գ���������Ҳ�ж�ʧ
  * @retval 3 ������ָ��
  * @note 
  * - ʹ��lwrb�����ringbuffer��
  * - ��len > RX_BUFFER_SIZEʱ��Ϊ��ֹд��Խ�磬��ǿ�нض�ΪRX_BUFFER_SIZE��С��
  * - ��ringbuffer�ռ䲻�㣬������ `lwrb_skip()` ���������ݣ����ȱ����������ݡ�
  */
static uint8_t USART1_Put_Data_Into_Ringbuffer(const void* data, uint16_t len)
{
    uint8_t ret = 0;
    if (data == NULL) return ret = 3;
    
    lwrb_sz_t freeSpace = lwrb_get_free((lwrb_t*)&g_Usart1RxRBHandler); // ringbufferʣ��ռ�
    
    if (len < RX_BUFFER_SIZE) { // �����ݳ���С��ringbuffer��������
        if (len <= freeSpace) { // �㹻��ʣ��ռ�
            lwrb_write((lwrb_t*)&g_Usart1RxRBHandler, data, len); // �����ݷ���ringbuffer
        } else { // û���㹻�Ŀռ䣬����Ҫ�������־�����
            lwrb_sz_t used = lwrb_get_full((lwrb_t*)&g_Usart1RxRBHandler); // ʹ���˶��ٿռ�
            lwrb_sz_t skip_len = len - freeSpace;
            if (skip_len > used) { // �ؼ������������ݳ��Ȳ��ܳ������������ܳ�������Խ�磨���磬��58bytes����������59bytes�����ֻ������58bytes)
                skip_len = used;
            }
            lwrb_skip((lwrb_t*)&g_Usart1RxRBHandler, skip_len); // Ϊ�˽����µ����ݣ��������־ɵ�����
            lwrb_write((lwrb_t*)&g_Usart1RxRBHandler, data, len); // �����ݷ���ringbuffer
            ret = 1;
        }
    } else if (len == RX_BUFFER_SIZE) { // �����ݳ��ȵ���ringbuffer��������
        if (freeSpace < RX_BUFFER_SIZE) {
            lwrb_reset((lwrb_t*)&g_Usart1RxRBHandler); // ����ringbuffer
            ret = 1;
        }
        lwrb_write((lwrb_t*)&g_Usart1RxRBHandler, data, len); // �����ݷ���ringbuffer
    } else { // �����ݳ��ȴ���ringbuffer��������������̫�󣬽��������RX_BUFFER_SIZE�ֽ�
        const uint8_t* byte_ptr = (const uint8_t*)data;
        data = (const void*)(byte_ptr + (len - RX_BUFFER_SIZE)); // ָ��ƫ��
        lwrb_reset((lwrb_t*)&g_Usart1RxRBHandler);
        lwrb_write((lwrb_t*)&g_Usart1RxRBHandler, data, RX_BUFFER_SIZE);
        ret = 2;
    }
    
    return ret;
}

/**
  * @brief  ���� DMA1 ͨ��5������ USART1 RX ��ѭ������
  * @note   ������������²�����
  *         1. ʹ�� DMA1 ʱ�ӣ������� DMA1_Channel5 �ж����ȼ���ʹ���ж�  
  *         2. ���� DMA ͨ��5�����ȴ�����ȫ�ر�  
  *         3. ���������ַ (CPAR=USART1->DR)���洢����ַ (CMAR=rx_buffer)  
  *            �Լ��������ݳ��� (CNDTR=RX_BUFFER_SIZE)  
  *         4. ���� CCR �Ĵ������������
  *            - DIR   = 0 (���� �� �洢��)
  *            - CIRC  = 1 (ѭ��ģʽ)
  *            - PINC  = 0 (�����ַ������)
  *            - MINC  = 1 (�洢����ַ����)
  *            - PSIZE = 00 (�������ݿ�� 8 λ)
  *            - MSIZE = 00 (�洢�����ݿ�� 8 λ)
  *            - PL    = 11 (���ȼ����ǳ���)
  *            - MEM2MEM = 0 (�Ǵ洢�����洢��ģʽ)
  *            - ��������ж� (TCIE) �� ���봫���ж� (HTIE) ʹ��
  *         5. ʹ�� DMA ͨ��5������ѭ������
  * @retval None
  */
static void DMA1_Channel5_Configure(void)
{
    // ʱ��
    RCC->AHBENR |= (1UL << 0UL); // ����DMA1ʱ��
    // ����ȫ���ж�
    NVIC_SetPriority(DMA1_Channel5_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0));
    NVIC_EnableIRQ(DMA1_Channel5_IRQn);
    /* 1. ����DMAͨ��5���ȴ�����ȫ�ر� */
    DMA1_Channel5->CCR &= ~(1UL << 0);  // ���ENλ
    while(DMA1_Channel5->CCR & 1UL);    // �ȴ�DMAͨ��5�ر�

    /* 2. ���������ַ�ʹ洢����ַ */
    DMA1_Channel5->CPAR = (uint32_t)&USART1->DR;  // �����ַΪUSART1���ݼĴ���
    DMA1_Channel5->CMAR = (uint32_t)rx_buffer;    // �洢����ַΪrx_buffer
    DMA1_Channel5->CNDTR = RX_BUFFER_SIZE;        // �������ݳ���Ϊ��������С
    /* 3. ����DMA1ͨ��5 CCR�Ĵ���
         - DIR   = 0��������洢��
         - CIRC  = 1��ѭ��ģʽ
         - PINC  = 0�������ַ������
         - MINC  = 1���洢����ַ����
         - PSIZE = 00���������ݿ��8λ
         - MSIZE = 00���洢�����ݿ��8λ
         - PL    = 10�����ȼ���Ϊ��
         - MEM2MEM = 0���Ǵ洢�����洢��ģʽ
    */
    DMA1_Channel5->CCR = 0;  // ���֮ǰ������
    DMA1_Channel5->CCR |= (1UL << 5);       // ʹ��ѭ��ģʽ (CIRC��bit5)
    DMA1_Channel5->CCR |= (1UL << 7);       // ʹ�ܴ洢������ (MINC��bit7)
    DMA1_Channel5->CCR |= (3UL << 12);      // �������ȼ�Ϊ�ǳ��� (PL��Ϊ��11����bit12-13)
    
    // ���Ӵ�������봫������ж�
    DMA1_Channel5->CCR |= (1UL << 1);       // ��������ж� (TCIE)
    DMA1_Channel5->CCR |= (1UL << 2);       // ��������ж� (HTIE)
    /* 4. ʹ��DMAͨ��5 */
    DMA1_Channel5->CCR |= 1UL;  // ��ENλ����ͨ��
}

/**
  * @brief  ���� DMA1 ͨ��4������ USART1 TX ���ڴ浽���贫��
  * @note   ������������²��裺
  *         1. ʹ�� DMA1 ʱ�� (RCC->AHBENR.DMA1EN, bit0)  
  *         2. ���ò����� DMA1_Channel4 �жϣ����ȼ�Ϊ 2 (NVIC)  
  *         3. ���ô��䷽��Ϊ�ڴ浽���� (CCR.DIR=1, bit4)  
  *         4. ����ͨ�����ȼ�Ϊ�� (CCR.PL=10, bits12�C13)  
  *         5. �ر�ѭ��ģʽ (CCR.CIRC=0, bit5)  
  *         6. ���õ�ַ����ģʽ�������ַ������ (CCR.PINC=0, bit6)��  
  *            �洢����ַ���� (CCR.MINC=1, bit7)  
  *         7. ������洢�����ݿ�Ⱦ�Ϊ 8 λ (CCR.PSIZE=00, bits8�C9;  
  *            CCR.MSIZE=00, bits10�C11)  
  *         8. ʹ�ܴ�������ж� (CCR.TCIE=1, bit1)  
  * @retval None
  */
static void DMA1_Channel4_Configure(void)
{
    // ����ʱ��
    RCC->AHBENR |= (1UL << 0UL); // ����DMA1ʱ��
    // ���ò�����ȫ���ж�
    NVIC_SetPriority(DMA1_Channel4_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),2, 0));
    NVIC_EnableIRQ(DMA1_Channel4_IRQn);
    // ���ݴ��䷽��
    DMA1_Channel4->CCR &= ~(1UL << 14UL); // ���赽�洢��ģʽ
    DMA1_Channel4->CCR |= (1UL << 4UL); // DIRλ����1���ڴ浽���裩
    // ͨ�����ȼ�
    DMA1_Channel4->CCR &= ~(3UL << 12UL); // ���PLλ
    DMA1_Channel4->CCR |= (2UL << 12UL);  // PLλ����10�����ȼ���
    // ѭ��ģʽ
    DMA1_Channel4->CCR &= ~(1UL << 5UL);  // ���CIRCλ���ر�ѭ��ģʽ
    // ����ģʽ
    DMA1_Channel4->CCR &= ~(1UL << 6UL);  // ����洢��ַ������
    DMA1_Channel4->CCR |= (1UL << 7UL);   // �洢����ַ����
    // ���ݴ�С
    DMA1_Channel4->CCR &= ~(3UL << 8UL);  // �������ݿ��8λ�����PSIZEλ���൱��00
    DMA1_Channel4->CCR &= ~(3UL << 10UL); // �洢�����ݿ��8λ�����MSIZEλ���൱��00
    // �ж�
    DMA1_Channel4->CCR |= (1UL << 1UL);   // ������������ж�
}

/**
  * @brief  USART1 ��������
  * @note   �������ԼĴ�����ʽ��� USART1 ״̬�Ĵ�����SR���еĴ����־��
  *            - PE  (λ0)����żУ�����  
  *            - FE  (λ1)��֡����  
  *            - NE  (λ2)����������  
  *            - ORE (λ3)�����չ��ش���  
  *         ����һ�����־����λ����ͨ�����ζ�ȡ SR �� DR ������д����־��
  * @retval 0 �޴���  
  * @retval 1 ��⵽���������
  */
__STATIC_INLINE uint8_t USART1_Error_Handler(void)
{
    // �Ĵ�����ʽ���������־��PE��FE��NE��ORE�ֱ�λ0~3��
    if (USART1->SR & ((1UL << 0) | (1UL << 1) | (1UL << 2) | (1UL << 3))) {
        // ��������־
        volatile uint32_t tmp = USART1->SR;
        tmp = USART1->DR;
        (void)tmp;
        return 1;
    } else {
        return 0;
    }
}

/**
  * @brief  DMA1 ͨ��4 ��������
  * @note   �ԼĴ�����ʽ��� DMA1 ISR �Ĵ����еĴ�������־ TEIF4 (ISR bit15)��
  *         ����⵽������ִ�����²�����
  *           - �� IFCR �Ĵ�����д 1 ��� TEIF4 ��־ (IFCR bit15)
  *           - ���� DMA1 ͨ��4��CCR.EN = 0��
  *           - ��� USART1 CR3 �Ĵ����е� DMAT λ��bit7����ֹͣ DMA ��������
  *           - ��ȫ�ֱ�־ tx_dma_busy ���㣬�������·��� DMA ����
  * @retval 0 �޴���
  * @retval 1 ��⵽��������Ѵ���
  */
__STATIC_INLINE uint8_t DMA1_Channel4_Error_Handler(void)
{
    // ��鴫�����TE����־������TE��Ӧλ(1UL << 15)������ݾ���оƬ�ο��ֲ�ȷ�ϣ�
    if (DMA1->ISR & (1UL << 15)) {
        // ���TE�����־
        DMA1->IFCR |= (1UL << 15);
        // ����DMAͨ��4
        DMA1_Channel4->CCR &= ~(1UL << 0);
        // ���USART1��DMATλ
        USART1->CR3 &= ~(1UL << 7);
        // ������ͱ�־����
        tx_dma_busy = 0;
        return 1;
    } else {
        return 0;
    }
}

/**
  * @brief  DMA1 ͨ��5 ��������
  * @note   �ԼĴ�����ʽ��� DMA1 ISR �Ĵ����еĴ�������־ TEIF5 (����Ϊ bit19)��
  *         ����⵽���������²��账��
  *           1. �� IFCR �Ĵ�����д 1 ��� TEIF5 ��־ (IFCR bit19)
  *           2. ���� DMA1 ͨ��5��CCR.EN = 0��
  *           3. ���ô�������Ĵ��� CNDTR Ϊ RX_BUFFER_SIZE
  *           4. ����ʹ�� DMA1 ͨ��5��CCR.EN = 1��
  * @retval 0 �޴���
  * @retval 1 ��⵽��������Ѵ���
  */
__STATIC_INLINE uint8_t DMA1_Channel5_Error_Hanlder(void)
{
    // ��鴫�����TE����־������TE��Ӧλ(1UL << 19)����ȷ�Ͼ���λ��
    if (DMA1->ISR & (1UL << 19)) {
        // ��������־
        DMA1->IFCR |= (1UL << 19);
        // ����DMAͨ��5
        DMA1_Channel5->CCR &= ~(1UL << 0);
        // ���ô������
        DMA1_Channel5->CNDTR = RX_BUFFER_SIZE;
        // ����ʹ��DMAͨ��5
        DMA1_Channel5->CCR |= 1UL;
        return 1;
    } else {
        return 0;
    }
}

/**
  * @brief  USART1 TX DMA1_Channel4 �������/�����жϴ���
  * @note   - �ȵ��� DMA1_Channel4_Error_Handler() ��鲢���������  
  *         - ���޴����Ҽ�⵽������� (ISR.TCIF4, bit13)����  
  *           1. �� IFCR ��д1��� TCIF4 ��־ (IFCR bit13)  
  *           2. ���� DMA ͨ��4��CCR.EN = 0��  
  *           3. ��� USART1 CR3 �е� DMAT λ (bit7)��ֹͣ DMA ��������  
  *           4. �� tx_dma_busy ��־�� 0��֪ͨ���������  
  * @retval None
  */
__STATIC_INLINE void USART1_TX_DMA1_Channel4_Interrupt_Handler(void)
{
    if (DMA1_Channel4_Error_Handler()) { // ��ش������
        dma1Channel4Error++;
    } else if (DMA1->ISR & (1UL << 13)) {
        // ���DMA������ɱ�־����IFCR�Ĵ�����д1�����Ӧ��־
        DMA1->IFCR |= (1UL << 13);
        // ����DMAͨ��4�����CCR�Ĵ�����ENλ��λ0��
        DMA1_Channel4->CCR &= ~(1UL << 0);
        // ���USART1��DMATλ���ر�DMA��������CR3�Ĵ�����λ7��
        USART1->CR3 &= ~(1UL << 7);
        // ���DMA�������
        tx_dma_busy = 0;
    }
}

/**
  * @brief  ��DMA���յ��������ݿ�����ringbuffer�У�������ѭ������DMA��ʽ��
  * 
  * ����������DMA���ջ��λ�����������ͬ������DMA�½��յ������ݴ�rx_buffer�п��������ringbuffer��
  * ֧�ִ���DMA������ָ�뻷�Ƶ��������֤���ݲ���ʧ��˳����ȷ��
  * 
  * @note
  * - ������������DMA����жϣ�������ж�/��ʱ���ȣ��е��ñ��������������ݻᱻ�����ݸ��Ƕ�ʧ��
  * - ͨ��`LL_DMA_GetDataLength()`��ȡ��ǰDMAʣ�ഫ���������Ƴ���ǰӲ��дָ�롣
  * - g_DMARxLastPos��ȫ�ֱ�����һ���Ѷ�λ�ã������ظ�����
  *
  * @par ע������
  * - ��`curr_pos < last_pos`��˵��DMAдָ���ѻ��ƣ���Ҫ�����ηֱ𿽱����ݡ�
  * - ����������������ͬ�������漰DMAʹ��/���ü��жϱ�־����
  */
__STATIC_INLINE void USART1_DMA_RX_Copy(void)
{
    uint16_t bufsize = sizeof(rx_buffer);
    uint16_t curr_pos = bufsize - DMA1_Channel5->CNDTR; // ���㵱ǰ��дָ��λ��
    uint16_t last_pos = g_DMARxLastPos; // ��һ�ζ�ָ��λ��
    
    if (curr_pos != last_pos) {
        if (curr_pos > last_pos) {
            // ��ͨ�����δ����
            USART1_Put_Data_Into_Ringbuffer((uint8_t*)rx_buffer + last_pos, curr_pos - last_pos);
            g_Usart1_RXCount += (curr_pos - last_pos);
        } else {
            // ���ƣ������δ���
            USART1_Put_Data_Into_Ringbuffer((uint8_t*)rx_buffer + last_pos, bufsize - last_pos);
            USART1_Put_Data_Into_Ringbuffer((uint8_t*)rx_buffer, curr_pos);
            g_Usart1_RXCount += (bufsize - last_pos) + curr_pos;
        }
    }
    g_DMARxLastPos = curr_pos; // ���¶�ָ��λ��
}

/**
  * @brief  USART1 RX DMA1_Channel5 �봫��/�������/�����жϴ���
  * @note   - �ȵ��� DMA1_Channel5_Error_Hanlder() ��鲢���������  
  *         - ����⵽�봫�� (ISR.HTIF5, bit18)��  
  *           1. �� IFCR ��д1��� HTIF5 ��־ (IFCR bit18)  
  *           2. �ۼ� RX_BUFFER_SIZE/2 �ֽڵ� g_Usart1_RXCount  
  *           3. ��������ǰ��������д�� ringbuffer  
  *         - ����⵽������� (ISR.TCIF5, bit17)��  
  *           1. �� IFCR ��д1��� TCIF5 ��־ (IFCR bit17)  
  *           2. �ۼ� RX_BUFFER_SIZE/2 �ֽڵ� g_Usart1_RXCount  
  *           3. �����������������д�� ringbuffer  
  * @retval None
  */
__STATIC_INLINE void USART1_RX_DMA1_Channel5_Interrupt_Handler(void)
{
    if (DMA1_Channel5_Error_Hanlder()) { // ��ش������
        dma1Channel5Error++;
    } else if (DMA1->ISR & (1UL << 18)) { // �봫���ж�
        DMA1->IFCR |= (1UL << 18);        // �����־
        USART1_DMA_RX_Copy();             // ��DMA���յ����ݷ���ringbuffer
    } else if (DMA1->ISR & (1UL << 17)) { // ��������ж�
        DMA1->IFCR |= (1UL << 17);        // �����־
        USART1_DMA_RX_Copy();             // ��DMA���յ����ݷ���ringbuffer
    }
}

/**
  * @brief  USART1 IDLE �жϼ������жϴ���
  * @note   - ���� USART1_Error_Handler() ��Ⲣ��� PE/FE/NE/ORE ����  
  *         - ����⵽ IDLE (SR.IDLE, bit4)��  
  *           1. �� SR��DR ��� IDLE ��־  
  *           2. ���� DMA1_Channel5��CCR.EN = 0������ȡ CNDTR ʣ���ֽ���  
  *           3. ���㱾�ν������ݳ��ȣ���д�� ringbuffer  
  *           4. ���� CNDTR Ϊ RX_BUFFER_SIZE ������ʹ��ͨ��  
  * @retval None
  */
__STATIC_INLINE void USART1_RX_Interrupt_Handler(void)
{
    if (USART1_Error_Handler()) { // ��ش��ڴ���
        usart1Error++; // �д��󣬼�¼�¼�
    } else if (USART1->SR & (1UL << 4)) { // ���USART1 SR�Ĵ�����IDLE��־��bit4��
        uint32_t tmp;
        // ���IDLE��־���ȶ�SR�ٶ�DR
        tmp = USART1->SR;
        tmp = USART1->DR;
        (void)tmp;
        USART1_DMA_RX_Copy();  // ��DMA���յ����ݷ���ringbuffer
    }
}

/**
  * @brief  ʹ��DMA�����ַ���������USART1_TX��Ӧ��DMA1ͨ��4
  * @param  data: ����������ָ�루����ָ��������ͻ�������
  * @param  len:  ���������ݳ���
  * @retval None
  */
void USART1_SendString_DMA(const uint8_t *data, uint16_t len)
{
    if (len == 0 || len > TX_BUFFER_SIZE) { // ������DMA����0���ֽڣ��ᵼ��û�취���뷢������жϣ�Ȼ�������������
        return;
    }
    // �ȴ���һ��DMA������ɣ�Ҳ������ӳ�ʱ���ƣ�
    while(tx_dma_busy);
    tx_dma_busy = 1; // ���DMA���ڷ���
    
    // ���DMAͨ��4����ʹ�ܣ����Ƚ����Ա���������
    if(DMA1_Channel4->CCR & 1UL) { // ���ENλ��bit0���Ƿ���λ
        DMA1_Channel4->CCR &= ~1UL;  // ����DMAͨ��4�����ENλ��
        while(DMA1_Channel4->CCR & 1UL); // �ȴ�DMAͨ��4��ȫ�ر�
    }
    // ����DMAͨ��4���ڴ��ַ�������ַ�����ݴ��䳤��
    DMA1_Channel4->CMAR  = (uint32_t)data;         // �����ڴ��ַ
    DMA1_Channel4->CPAR  = (uint32_t)&USART1->DR;  // ���������ַ
    DMA1_Channel4->CNDTR = len;                    // ���ô������ݳ���
    // ����USART1��DMA��������CR3��DMAT����7λ����1
    USART1->CR3 |= (1UL << 7UL);
    // ����DMAͨ��4������ENλ����DMA����
    DMA1_Channel4->CCR |= 1UL;
}

/**
  * @brief   ������д�� USART1 ���� ringbuffer ��
  * @param[in] data ָ��Ҫд��Ĵ��������ݻ�����
  * @param[in] len  Ҫд������ݳ��ȣ���λ���ֽڣ�
  * @retval  0 ���ݳɹ�д�룬�����ݶ���
  * @retval  1 ringbuffer �ռ䲻�㣬�������־�����������������
  * @retval  2 ���ݳ��ȳ��� ringbuffer ����������������� TX_BUFFER_SIZE �ֽ�
  * @retval  3 ������ָ��
  * @note
  *   - ʹ�� lwrb ��������� ringbuffer��g_Usart1TxRBHandler��
  *   - �� len > TX_BUFFER_SIZE����ض�Ϊ��� TX_BUFFER_SIZE �ֽ�
  *   - ���ռ䲻�㣬������ lwrb_skip() ����������
  */
uint8_t USART1_Put_TxData_To_Ringbuffer(const void* data, uint16_t len)
{
    uint8_t ret = 0;
    if (data == NULL) {
        return 3;
    }

    lwrb_sz_t capacity  = TX_BUFFER_SIZE;
    lwrb_sz_t freeSpace = lwrb_get_free((lwrb_t*)&g_Usart1TxRBHandler); // ��ȡʣ��ռ�

    if (len < capacity) {
        if (len <= freeSpace) {
            // ֱ��д��
            lwrb_write((lwrb_t*)&g_Usart1TxRBHandler, data, len);
        } else {
            // �ռ䲻�㣬����������
            lwrb_sz_t used     = lwrb_get_full((lwrb_t*)&g_Usart1TxRBHandler);
            lwrb_sz_t skip_len = len - freeSpace;
            if (skip_len > used) {
                skip_len = used;
            }
            lwrb_skip((lwrb_t*)&g_Usart1TxRBHandler, skip_len);
            lwrb_write((lwrb_t*)&g_Usart1TxRBHandler, data, len);
            ret = 1;
        }
    } else if (len == capacity) {
        // �պõ�������
        if (freeSpace < capacity) {
            lwrb_reset((lwrb_t*)&g_Usart1TxRBHandler);
            ret = 1;
        }
        lwrb_write((lwrb_t*)&g_Usart1TxRBHandler, data, len);
    } else {
        // len > ��������������� capacity �ֽ�
        const uint8_t* ptr = (const uint8_t*)data + (len - capacity);
        lwrb_reset((lwrb_t*)&g_Usart1TxRBHandler);
        lwrb_write((lwrb_t*)&g_Usart1TxRBHandler, ptr, capacity);
        ret = 2;
    }

    return ret;
}

void USART1_Module_Run(void)
{
    /* 1. �������ݴ���ʾ���� */
    if (lwrb_get_full((lwrb_t*)&g_Usart1RxRBHandler)) {
        // TODO: ������Ľ��մ����������磺
        // Process_Usart1_RxData();
    }
    
    /* 2. �������ݣ������� & DMA ���� */
    uint16_t available = lwrb_get_full((lwrb_t*)&g_Usart1TxRBHandler);
    if (available && 0x00 == USART1_Get_TX_DMA_Busy()) {
        uint16_t len = (available > TX_BUFFER_SIZE) ? TX_BUFFER_SIZE : (uint16_t)available; // �����������
        lwrb_read((lwrb_t*)&g_Usart1TxRBHandler, (uint8_t*)tx_buffer, len); // �ӻ��λ�������ȡ�� DMA ������
        g_Usart1_TXCount += len; // ͳ�Ʒ�������
        USART1_SendString_DMA((uint8_t*)tx_buffer, len); // ���� DMA ����
    }
}

/**
  * @brief  ���� USART1 ���裬����ʱ�ӡ�GPIO�����ڲ������ж��� DMA
  * @note   �����������²������ USART1 �ĳ�ʼ����
  *         1. ��ʼ���շ����λ�������lwrb��  
  *         2. ʹ�� USART1 �� GPIOA ����ʱ��  
  *         3. ���� PA9��TX��Ϊ 10MHz �������������PA10��RX��Ϊ��������  
  *         4. ���� USART1 ȫ���жϣ����ȼ�Ϊ 0  
  *         5. ���ò����ʣ�72MHz PCLK2��������16��BRR=39.0625  
  *         6. �������ݸ�ʽ��8 λ���ݡ�1 ��ֹͣλ����У��  
  *         7. ʹ�ܷ��ͣ�TE������գ�RE������  
  *         8. �ر�Ӳ�����ء�LIN��ʱ����������ܿ���IrDA����˫��ģʽ  
  *         9. ʹ�ܿ����жϣ�IDLEIE��  
  *        10. ʹ�� DMA ��������DMAR���������� DMA1_Channel5/4 ��ʼ������  
  *        11. ���ʹ�� USART��UE��  
  * @retval None
  */
void USART1_Configure(void) {
    
    /* ringbuffer */
    lwrb_init((lwrb_t*)&g_Usart1RxRBHandler, (uint8_t*)g_Usart1RxRB, sizeof(g_Usart1RxRB) + 1); // RX ringbuffer��ʼ��
    lwrb_init((lwrb_t*)&g_Usart1TxRBHandler, (uint8_t*)g_Usart1TxRB, sizeof(g_Usart1TxRB) + 1); // TX ringbuffer��ʼ��
    
    /* 1. ʹ������ʱ�� */
    // RCC->APB2ENR �Ĵ������� APB2 ����ʱ��
    RCC->APB2ENR |= (1UL << 14UL); // ʹ�� USART1 ʱ�� (λ 14)
    RCC->APB2ENR |= (1UL << 2UL);  // ʹ�� GPIOA ʱ�� (λ 2)
    /* 2. ���� GPIO (PA9 - TX, PA10 - RX) */
    // GPIOA->CRH �Ĵ������� PA8-PA15 ��ģʽ������
    // PA9: ����������� (ģʽ: 10, CNF: 10)
    GPIOA->CRH &= ~(0xF << 4UL);        // ���� PA9 ������λ (λ 4-7)
    GPIOA->CRH |= (0xA << 4UL);         // PA9: 10MHz ����������� (MODE9 = 10, CNF9 = 10)
    // PA10: �������� (ģʽ: 00, CNF: 01)
    GPIOA->CRH &= ~(0xF << 8UL);        // ���� PA10 ������λ (λ 8-11)
    GPIOA->CRH |= (0x4 << 8UL);         // PA10: ����ģʽ���������� (MODE10 = 00, CNF10 = 01)
    // ����USART1ȫ���ж�
    NVIC_SetPriority(USART1_IRQn, NVIC_EncodePriority(NVIC_GetPriorityGrouping(),0, 0)); // ���ȼ�1�����ȼ�Խ���൱��Խ���ȣ�
    NVIC_EnableIRQ(USART1_IRQn);
    /* 3. ���� USART1 ���� */
    // (1) ���ò����� 115200 (ϵͳʱ�� 72MHz, ������ 16)
    // �����ʼ���: USART_BRR = fPCLK / (16 * BaudRate)
    // 72MHz / (16 * 115200) = 39.0625
    // ��������: 39 (0x27), С������: 0.0625 * 16 = 1 (0x1)
    USART1->BRR = (39 << 4UL) | 1;      // BRR = 0x271 (39.0625)
    // (2) ��������֡��ʽ (USART_CR1 �� USART_CR2)
    USART1->CR1 &= ~(1UL << 12UL);      // M λ = 0, 8 λ����
    USART1->CR2 &= ~(3UL << 12UL);      // STOP λ = 00, 1 ��ֹͣλ
    USART1->CR1 &= ~(1UL << 10UL);      // û��żУ��
    // (3) ���ô��䷽�� (�շ�˫��)
    USART1->CR1 |= (1UL << 3UL);        // TE λ = 1, ʹ�ܷ���
    USART1->CR1 |= (1UL << 2UL);        // RE λ = 1, ʹ�ܽ���
    // (4) ����Ӳ������ (USART_CR3)
    USART1->CR3 &= ~(3UL << 8UL);       // CTSE �� RTSE λ = 0, ������
    // (5) �����첽ģʽ (����޹�ģʽλ)
    USART1->CR2 &= ~(1UL << 14UL);      // LINEN λ = 0, ���� LIN ģʽ
    USART1->CR2 &= ~(1UL << 11UL);      // CLKEN λ = 0, ����ʱ�����
    USART1->CR3 &= ~(1UL << 5UL);       // SCEN λ = 0, �������ܿ�ģʽ
    USART1->CR3 &= ~(1UL << 1UL);       // IREN λ = 0, ���� IrDA ģʽ
    USART1->CR3 &= ~(1UL << 3UL);       // HDSEL λ = 0, ���ð�˫��
    // (6) �ж�
    USART1->CR1 |= (1UL << 4UL);        // ʹ��USART1�����ж� (IDLEIE, λ4)
    // (7) ����DMA�Ľ�������
    USART1->CR3 |= (1UL << 6UL);        // ʹ��USART1��DMA��������DMAR��λ6�� 
    // (7) ���� USART
    USART1->CR1 |= (1UL << 13UL);       // UE λ = 1, ���� USART
    
    // DMA��ʼ��
    DMA1_Channel5_Configure();
    DMA1_Channel4_Configure();
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
    DMA1_Channel4->CCR &= ~(1UL << 0); // ����DMA1ͨ��4�����CCR��ENλ��λ0��
    while (DMA1_Channel4->CCR & 1UL) { // �ȴ�DMA1ͨ��4��ȫ�رգ��������ӳ�ʱ����
        // ��ѭ���ȴ�
    }
    DMA1_Channel5->CCR &= ~(1UL << 0); // ����DMA1ͨ��5�����CCR��ENλ��λ0��
    while (DMA1_Channel5->CCR & 1UL) { // �ȴ�DMA1ͨ��5��ȫ�رգ��������ӳ�ʱ����
        // ��ѭ���ȴ�
    }
    NVIC_DisableIRQ(USART1_IRQn); // ����USART1ȫ���жϣ��������������в����µ��жϸ���
    USART1->CR3 &= ~(1UL << 6); // ����USART1��DMA�����������CR3��DMARλ��λ6��
    USART1->CR1 &= ~(1UL << 13); // ����USART1�����CR1��UEλ��λ13��
    
    // ��SR��DR���������Ĵ����־������IDLE��ORE��NE��FE��PE��
    volatile uint32_t tmp = USART1->SR;
    tmp = USART1->DR;
    (void)tmp;
    
    for (volatile uint32_t i = 0; i < 1000; i++); // ��ѡ��������ʱ��ȷ��USART1��ȫ�ر�
    tx_dma_busy = 0; // ��λ���ͱ�־����������
    
    /* ���³�ʼ��USART1��DMA1 */
    USART1_Configure();
}

/**
  * @brief This function handles DMA1 channel4 global interrupt.
  */
void DMA1_Channel4_IRQHandler(void)
{
    USART1_TX_DMA1_Channel4_Interrupt_Handler();
}

/**
  * @brief This function handles DMA1 channel5 global interrupt.
  */
void DMA1_Channel5_IRQHandler(void)
{
    USART1_RX_DMA1_Channel5_Interrupt_Handler();
}

/**
  * @brief This function handles USART1 global interrupt.
  */
void USART1_IRQHandler(void)
{
    USART1_RX_Interrupt_Handler();
}




