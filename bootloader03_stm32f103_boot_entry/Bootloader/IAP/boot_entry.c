#include "boot_entry.h"
#include "flash_map.h"

/**
  * @brief  �ϵ磬ϵͳ���ڳ�ʼ���ص���main()ǰ�Զ����ã�
  * @note
  *   - ������ͨ�� GCC/ARMCC �� @c __attribute__((constructor)) �������Σ�ϵͳ������main()ִ��ǰ�Զ����С�
  *   - �����ڽ���������־�ض��򡢻�����⡢�̼�������У�顢������־�жϵ�ȫ�ֳ�ʼ��������
  *   - ����Ŀ��չ���������Ʊ��������ݣ���������ò����������ʼ���Ĺؼ����롣
  *
  * @attention
  *   - ʹ�� @c __attribute__((constructor)) ����Ҫ�󹤳����ӽű�/�����ļ���ȷ֧�� .init_array �Ρ�
  *   - ���������������ű���֧�ָû��ƣ��뽫�ú��������ֶ����� main() ��ǰ����á�
  *
  * @see    Retarget_RTT_Init(), log_printf()
  */
__attribute__((constructor))
static void _SystemStart(void)
{
    Retarget_RTT_Init(); //! RTT�ض���printf
    
    log_printf("System Start!\n");
    
    /*! ��鸴λ���±�־ */

    /*! �����������ģʽ�ж� */
    /*! ���Ӽ��һ��������ǿ�ƽ���Bootloader����ģʽ�ȵ� */
}

