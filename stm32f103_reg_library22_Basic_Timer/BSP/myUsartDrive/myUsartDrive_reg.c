#include "myUsartDrive/myUsartDrive_reg.h"

volatile uint8_t rx_buffer[RX_BUFFER_SIZE];
volatile uint8_t rx_complete = 0;
volatile uint16_t recvd_length = 0;

volatile uint8_t tx_buffer[TX_BUFFER_SIZE];
volatile uint8_t tx_dma_busy = 0;

volatile uint16_t usart1Error = 0;
volatile uint16_t dma1Channel4Error = 0;
volatile uint16_t dma1Channel5Error = 0;


__STATIC_INLINE void Enable_Peripherals_Clock(void)
{
    SET_BIT(RCC->APB2ENR, 1UL << 0UL);  // ����AFIOʱ��  // һ��Ĺ��̶�Ҫ��
    SET_BIT(RCC->APB1ENR, 1UL << 28UL); // ����PWRʱ��   // һ��Ĺ��̶�Ҫ��
    SET_BIT(RCC->APB2ENR, 1UL << 5UL);  // ����GPIODʱ�� // ����ʱ��
    SET_BIT(RCC->APB2ENR, 1UL << 2UL);  // ����GPIOAʱ�� // SWD�ӿ�
    __NOP(); // ��΢��ʱһ����
}

// ����USART1_RX��DMA1ͨ��5
__STATIC_INLINE void DMA1_Channel5_Configure(void)
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

// ����DMA1��ͨ��4����ͨģʽ���ڴ浽����(flash->USART1_TX)�����ȼ��ߣ��洢����ַ���������ݴ�С����8bit
__STATIC_INLINE void DMA1_Channel4_Configure(void)
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

__STATIC_INLINE void USART1_RX_DMA1_Channel5_Interrupt_Handler(void)
{
    if (DMA1_Channel5_Error_Hanlder()) { // ��ش������
        dma1Channel5Error++;
    } else if (DMA1->ISR & (1UL << 18)) { // �봫���ж�
        DMA1->IFCR |= (1UL << 18);
        // ǰ�뻺��������
        memcpy((void*)tx_buffer, (const void*)rx_buffer, RX_BUFFER_SIZE / 2);
        recvd_length = RX_BUFFER_SIZE / 2;
        rx_complete = 1;
    } else if (DMA1->ISR & (1UL << 17)) { // ��������ж�
        DMA1->IFCR |= (1UL << 17);
        // ��뻺��������
        memcpy((void*)tx_buffer, (const void*)(rx_buffer + RX_BUFFER_SIZE / 2), RX_BUFFER_SIZE / 2);
        recvd_length = RX_BUFFER_SIZE / 2;
        rx_complete = 1;
    }
}

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
        // ����DMA1ͨ��5�����CCR�Ĵ�����ENλ��bit0��
        DMA1_Channel5->CCR &= ~(1UL << 0);
        // ��ȡʣ���������
        uint16_t remaining = DMA1_Channel5->CNDTR;
        uint16_t count = 0;

        // ����ʣ���ֽ��жϵ�ǰ�����ĸ�����
        // ע�⣺Ϊ���⵱���ݳ��ȸպ�Ϊ512�ֽڻ�1024�ֽ�ʱ����DMA�봫��/��������жϳ�ͻ��
        // ��count��Ϊ0ʱ�Ž������ݸ���
        if (remaining > (RX_BUFFER_SIZE / 2)) {
            // ���ڽ���ǰ���������������� = (�ܳ��� - remaining)������512�ֽ�
            count = RX_BUFFER_SIZE - remaining;
            if (count != 0) {  // �����봫������жϳ�ͻ���ิ��һ��
                memcpy((void*)tx_buffer, (const void*)rx_buffer, count);
            }
        } else {
            // ǰ������������ǰ�ں��������������������� = (ǰ�������� - remaining)
            count = (RX_BUFFER_SIZE / 2) - remaining;
            if (count != 0) {  // �����봫������жϳ�ͻ���ิ��һ��
                memcpy((void*)tx_buffer, (const void*)rx_buffer + (RX_BUFFER_SIZE / 2), count);
            }
        }
        if (count != 0) {
            recvd_length = count;
            rx_complete = 1;
        }
        // ����DMA���գ�����CNDTR�Ĵ���ΪRX_BUFFER_SIZE
        DMA1_Channel5->CNDTR = RX_BUFFER_SIZE;
        // ����ʹ��DMA1ͨ��5������ENλ��bit0��
        DMA1_Channel5->CCR |= 1UL;
    }
}

/**
  * @brief  ʹ��DMA�����ַ���������USART1_TX��Ӧ��DMA1ͨ��4
  * @param  data: ����������ָ�루����ָ��������ͻ�������
  * @param  len:  ���������ݳ���
  * @retval None
  */
void USART1_SendString_DMA(const char *data, uint16_t len)
{
    if (len == 0) {
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

void USART1_Module_Run(void)
{
    if (rx_complete) {
        USART1_SendString_DMA((const char*)tx_buffer, recvd_length);
        rx_complete = 0; // �����־���ȴ���һ�ν���
    }
}

void USART1_Configure(void) {
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

void DMA1_Channel4_IRQHandler(void)
{
    USART1_TX_DMA1_Channel4_Interrupt_Handler();
}

void DMA1_Channel5_IRQHandler(void)
{
    USART1_RX_DMA1_Channel5_Interrupt_Handler();
}

void USART1_IRQHandler(void)
{
    USART1_RX_Interrupt_Handler();
}
































