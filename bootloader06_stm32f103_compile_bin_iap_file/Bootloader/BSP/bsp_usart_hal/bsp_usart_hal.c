/**
 * @file    bsp_usart_hal.c
 * @brief   STM32F1ϵ�� USART + DMA + RingBuffer HAL��ײ�����ʵ�֣���ʵ�����ɱ仺������
 * @author  Wallace.zhang
 * @version 2.0.0
 * @date    2025-05-23
 */

#include "bsp_usart_hal.h"

/**
  * @brief   ������ʽ������NUL��β�ַ����������ã���DMA��
  * @param   usart  ָ��USART�����ṹ���ָ��
  * @param   str    ָ����'\0'��β���ַ���
  * @note    ͨ��HAL��API���ֽڷ��ͣ��ײ����ѯTXEλ��USART_SR.TXE����
  * @retval  ��
  */
void USART_SendString_Blocking(USART_Driver_t* usart, const char* str)
{
    if (!usart || !str) return;
    HAL_UART_Transmit(usart->huart, (uint8_t*)str, strlen(str), 1000);
}

/**
  * @brief  ���ò�����USART��DMA���գ�����ģʽ��
  * @param  usart ָ��USART�����ṹ���ָ��
  * @note   ���뱣֤huart��hdma_rx��ͨ��CubeMX��ȷ��ʼ��
  *         - ���ñ�������ֹͣԭ��DMA��Ȼ����������DMA���������ν���
  *         - ʹ��USART��IDLE�жϣ�ʵ��ͻ��/������֡��Ч����
  * @retval ��
  */
static void USART_Received_DMA_Configure(USART_Driver_t *usart)
{
    if (!usart) return;
    HAL_DMA_Abort(usart->hdma_rx); //! �ȹر�
    HAL_UART_Receive_DMA(usart->huart, usart->rxDMABuffer, usart->rxBufSize); //! ��������DMA
    __HAL_UART_ENABLE_IT(usart->huart, UART_IT_IDLE); //! ʹ��IDLE�ж�
    usart->dmaRxLastPos = 0;
}

/**
  * @brief  ����DMA��ʽ���ڷ���
  * @param  usart ָ��USART�����ṹ���ָ��
  * @param  data  ָ������͵����ݻ�����
  * @param  len   �����͵������ֽ���
  * @note   ����ǰ��ȷ��txDMABusyΪ0������Ӧ�ȴ�ǰһ֡�������
  *         ������DMA�Զ����USART_DR�Ĵ�����ʵ�ָ�Ч�첽����
  * @retval ��
  */
static void USART_SendString_DMA(USART_Driver_t *usart, uint8_t *data, uint16_t len)
{
    if (!usart || !data || len == 0 || len > usart->txBufSize) return;
    while (usart->txDMABusy); // �ȴ�DMA����
    usart->txDMABusy = 1;
    HAL_UART_Transmit_DMA(usart->huart, data, len);
}

/**
 * @brief  д�����ݵ�����RingBuffer
 * @param  usart ָ��USART�����ṹ���ָ��
 * @param  data  ָ��Ҫд������ݻ�����
 * @param  len   Ҫд������ݳ��ȣ���λ���ֽڣ�
 * @retval 0  ���ݳɹ�д�룬�����ݶ���
 * @retval 1  ringbufferʣ��ռ䲻�㣬�������־�����������������
 * @retval 2  ���ݳ��ȳ���ringbuffer��������������������β����ȫ�������ݱ���գ�
 * @retval 3  ��������ָ��Ϊ��
 * @note
 * - ������ͨ��lwrb�����ringbuffer��
 * - ��len > ringbuffer����ʱ��ǿ�нضϣ�����������usart->rxRBBuffer�ֽڡ�
 * - ���ռ䲻�㣬�Զ�����lwrb_skip()�������־����ݡ�
 */
static uint8_t Put_Data_Into_Ringbuffer(USART_Driver_t *usart, const void *data, uint16_t len)
{
    //! �������ָ���Ƿ�Ϸ�
    if (!usart || !data) return 3;
    
    lwrb_t *rb = &usart->rxRB;
    uint16_t rb_size = usart->rxBufSize;
    //! ��ȡ��ǰRingBufferʣ��ռ�
    lwrb_sz_t free_space = lwrb_get_free(rb);
    
    //! �����������������С�ڡ����ڡ�����RingBuffer����
    uint8_t ret = 0;
    if (len < rb_size) {
        //! ����С��RingBuffer����
        if (len <= free_space) {
            //! �ռ���㣬ֱ��д��
            lwrb_write(rb, data, len);
        } else {
            //! �ռ䲻�㣬�趪�����־�����
            lwrb_sz_t used = lwrb_get_full(rb);
            lwrb_sz_t skip_len = len - free_space;
            if (skip_len > used) {
                skip_len = used;
            }
            lwrb_skip(rb, skip_len); //! ������������������
            lwrb_write(rb, data, len);
            ret = 1;
        }
    } else if (len == rb_size) { //! ���ݸպõ���RingBuffer����
        if (free_space < rb_size) {
            lwrb_reset(rb); //! �ռ䲻�㣬����RingBuffer
            ret = 1;
        }
        lwrb_write(rb, data, len);
    } else { //! ���ݳ���RingBuffer���������������rb_size�ֽ�
        const uint8_t *byte_ptr = (const uint8_t *)data;
        data = (const void *)(byte_ptr + (len - rb_size));
        lwrb_reset(rb);
        lwrb_write(rb, data, rb_size);
        ret = 2;
    }
    return ret;
}

/**
 * @brief  ��DMA���λ������������յ������ݵ�RingBuffer��֧�ֻ��ƣ�
 * @param  usart ָ��USART�����ṹ���ָ��
 * @note   ֧��IDLE��DMA HT/TC�ȶ��жϹ�ͬ����
 *         - ����������DMA���λ������������ݣ������˵�RingBuffer
 *         - ֧��һ���Ի�ֶΰ��ˣ�����������ʱ�Զ������δ���
 * @retval ��
 */
static void USART_DMA_RX_Copy(USART_Driver_t *usart)
{
    uint16_t bufsize  = usart->rxBufSize;
    uint16_t curr_pos = bufsize - __HAL_DMA_GET_COUNTER(usart->hdma_rx);
    uint16_t last_pos = usart->dmaRxLastPos;

    if (curr_pos != last_pos) {
        if (curr_pos > last_pos) {
        //! ��ͨ�����δ����
            Put_Data_Into_Ringbuffer(usart, usart->rxDMABuffer + last_pos, curr_pos - last_pos);
            usart->rxMsgCount += (curr_pos - last_pos);
        } else {
        //! ���ƣ������δ���
            Put_Data_Into_Ringbuffer(usart, usart->rxDMABuffer + last_pos, bufsize - last_pos);
            Put_Data_Into_Ringbuffer(usart, usart->rxDMABuffer, curr_pos);
            usart->rxMsgCount += (bufsize - last_pos) + curr_pos;
        }
        usart->dmaRxLastPos = curr_pos;
    }
}

/**
  * @brief  DMA���հ��˻���Buffer���Ƽ���IDLE��DMA�жϵȵ��ã�
  * @param  usart ָ��USART�����ṹ���ָ��
  * @retval ��
  */
void USART_DMA_RX_Interrupt_Handler(USART_Driver_t *usart)
{
    if (!usart) return;
    USART_DMA_RX_Copy(usart);
}

/**
  * @brief  ����IDLE�жϴ�������USARTx_IRQHandler�е��ã�
  * @param  usart ָ��USART�����ṹ���ָ��
  * @note   ��鲢���IDLE��־����ʱ����DMA����
  * @retval ��
  */
void USART_RX_IDLE_Interrupt_Handler(USART_Driver_t *usart)
{
    if (!usart) return;
    if (__HAL_UART_GET_FLAG(usart->huart, UART_FLAG_IDLE)) {
        __HAL_UART_CLEAR_IDLEFLAG(usart->huart);
        USART_DMA_RX_Copy(usart);
    }
}

/**
  * @brief  DMA������ɻص������û���HAL��TxCpltCallback�е��ã�
  * @param  usart ָ��USART�����ṹ���ָ��
  * @note   һ��Ҫ���ã������޷��ٴ�DMA����
  * @retval ��
  */
void USART_DMA_TX_Interrupt_Handler(USART_Driver_t *usart)
{
    if (!usart) return;
    usart->txDMABusy = 0;
}

/**
  * @brief  ������д��ָ��USART�����ķ��� RingBuffer ��
  * @param  usart  ָ��USART�����ṹ���ָ��
  * @param  data   ָ��Ҫд������ݻ�����
  * @param  len    Ҫд������ݳ��ȣ��ֽڣ�
  * @retval  0  ���ݳɹ�д�룬�����ݶ���
  * @retval  1  ringbuffer �ռ䲻�㣬�������־�����������������
  * @retval  2  ���ݳ��ȳ��� ringbuffer ������������������ TX_BUFFER_SIZE �ֽ�
  * @retval  3  ��������ָ��Ϊ��
  * @note
  * - ʹ�� lwrb ��������� RingBuffer��usart->txRB����
  * - �� len > ringbuffer ���������Զ��ضϣ����������µ����ݡ�
  * - ���ռ䲻�㣬������ lwrb_skip() �������־����ݡ�
  */
uint8_t USART_Put_TxData_To_Ringbuffer(USART_Driver_t *usart, const void* data, uint16_t len)
{
    if (!usart || !data) return 3; //! �����������ָ����Ч��
    
    lwrb_t *rb = &usart->txRB;
    uint16_t capacity = usart->txBufSize;
    lwrb_sz_t freeSpace = lwrb_get_free(rb);
    uint8_t ret = 0;
    
    //! ���1�����ݳ���С��ringbuffer����
    if (len < capacity) {
        if (len <= freeSpace) {
            lwrb_write(rb, data, len); //! ʣ��ռ���㣬ֱ��д��
        } else {
            //! �ռ䲻�㣬�趪�����־�����
            lwrb_sz_t used = lwrb_get_full(rb);
            lwrb_sz_t skip_len = len - freeSpace;
            if (skip_len > used) skip_len = used;
            lwrb_skip(rb, skip_len);
            lwrb_write(rb, data, len);
            ret = 1;
        }
    } else if (len == capacity) { //! ���2�����ݳ��ȵ���ringbuffer����
        if (freeSpace < capacity) { //! ���ringbuffer��������
            lwrb_reset(rb);
            ret = 1;
        }
        lwrb_write(rb, data, len);
    } else { //! ���3�����ݳ��ȴ���ringbuffer��������������� capacity �ֽ�
        const uint8_t *ptr = (const uint8_t*)data + (len - capacity);
        lwrb_reset(rb);
        lwrb_write(rb, ptr, capacity);
        ret = 2;
    }
    return ret;
}

/**
 * @brief   ��ʼ��USART����������DMA��RingBuffer���ж�
 * @param   usart         ָ��USART�����ṹ���ָ��
 * @param   rxDMABuffer   DMA���ջ�����ָ��
 * @param   rxRBBuffer    ����RingBuffer������ָ��
 * @param   rxBufSize     ���ջ�������С
 * @param   txDMABuffer   DMA���ͻ�����ָ��
 * @param   txRBBuffer    ����RingBuffer������ָ��
 * @param   txBufSize     ���ͻ�������С
 * @retval  ��
 * @note    ����ͨ��CubeMX��ɴ��ڡ�DMA���Ӳ�����ú;����ֵ
 */
void USART_Config(USART_Driver_t *usart,
                  uint8_t *rxDMABuffer, uint8_t *rxRBBuffer, uint16_t rxBufSize,
                  uint8_t *txDMABuffer, uint8_t *txRBBuffer, uint16_t txBufSize)
{
    if (!usart) return;
    usart->rxDMABuffer = rxDMABuffer;
    usart->rxRBBuffer  = rxRBBuffer;
    usart->rxBufSize   = rxBufSize;
    lwrb_init(&usart->rxRB, usart->rxRBBuffer, usart->rxBufSize);

    usart->txDMABuffer = txDMABuffer;
    usart->txRBBuffer  = txRBBuffer;
    usart->txBufSize   = txBufSize;
    lwrb_init(&usart->txRB, usart->txRBBuffer, usart->txBufSize);

    USART_Received_DMA_Configure(usart); // ��ʼ��DMA RX

    usart->txDMABusy = 0;
    usart->dmaRxLastPos = 0;
    usart->rxMsgCount = 0;
    usart->txMsgCount = 0;
    usart->errorDMATX = 0;
    usart->errorDMARX = 0;
    usart->errorRX = 0;
}

/**
  * @brief  USARTģ����ѭ�����Ⱥ�����DMA + RingBuffer��Ч�շ���
  * @param  usart ָ��USART�����ṹ���ָ��
  * @note   ������ѭ����ʱ����1ms������
  *         - ��鷢��RingBuffer�Ƿ��д��������ݣ���DMA��ǰ����
  *         - ���������㣬�ӷ���RingBuffer��ȡһ�����ݵ�DMA���ͻ���������ͨ��DMA�����첽����
  *         - �Զ�ά���ѷ�������ͳ��
  * @retval ��
  */
void USART_Module_Run(USART_Driver_t *usart)
{
    if (!usart) return;
    uint16_t available = lwrb_get_full(&usart->txRB);
    if (available && usart->txDMABusy == 0) {
        uint16_t len = (available > usart->txBufSize) ? usart->txBufSize : available;
        lwrb_read(&usart->txRB, usart->txDMABuffer, len);
        usart->txMsgCount += len;
        USART_SendString_DMA(usart, usart->txDMABuffer, len);
    }
}

/**
  * @brief  ��ȡUSART����RingBuffer�еĿɶ��ֽ���
  * @param  usart ָ��USART�����ṹ���ָ��
  * @retval uint32_t �ɶ�ȡ�������ֽ���
  * @note   ͨ������ѭ�������ݽ���ǰ���ã������ж��Ƿ���Ҫ��ȡ���ݡ�
  */
uint32_t USART_Get_The_Existing_Amount_Of_Data(USART_Driver_t *usart)
{
    if (!usart) return 0;
    return lwrb_get_full(&usart->rxRB);
}

/**
  * @brief  ��USART����RingBuffer�ж�ȡһ���ֽ�����
  * @param  usart ָ��USART�����ṹ���ָ��
  * @param  data  ָ���Ŷ�ȡ����Ļ�����ָ��
  * @retval 1  ��ȡ�ɹ����������ݴ��� *data
  * @retval 0  ��ȡʧ�ܣ������ݻ�dataΪNULL��
  * @note   ����������������������ʱֱ�ӷ���0��
  */
uint8_t USART_Take_A_Piece_Of_Data(USART_Driver_t *usart, uint8_t* data)
{
    if (!usart || !data) return 0;
    return lwrb_read(&usart->rxRB, data, 1);
}

/**
  * @brief  DMA����������Զ��ָ�������������ͳ�ƣ�
  * @param  usart ָ��USART�����ṹ��
  * @param  dir   ����0=RX, 1=TX
  * @note   ��⵽DMA�������TE��ʱ���ã��Զ�����ͳ�Ʋ��ָ�
  *         RX������Զ�����DMA��TX������ȴ���ѭ�������·���
  * @retval ��
  */
void USART_DMA_Error_Recover(USART_Driver_t *usart, uint8_t dir)
{
    if (!usart) return;

    if (dir == 0) { //! RX����
        usart->errorDMARX++; //! DMA���մ������ */
        HAL_DMA_Abort(usart->hdma_rx);
        HAL_UART_Receive_DMA(usart->huart, usart->rxDMABuffer, usart->rxBufSize);
        //! ���Լ��뼫������µ�USART��λ��
    } else { //! TX����
        usart->errorDMATX++; //! DMA���ʹ������ */
        HAL_DMA_Abort(usart->hdma_tx);
        //! һ��ȴ���ѭ�������µ�DMA����
    }
    //! �ɲ��뱨������־
}




