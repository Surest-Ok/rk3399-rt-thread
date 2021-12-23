/*
 * Copyright (c) 2006-2021, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author         Notes
 * 2020-04-16     bigmagic       first version
 */

#include <rthw.h>
#include <rtthread.h>
#include <gicv3.h>
#include <board.h>
#include <armv8.h>

#define MAX_HANDLERS                512
#define GIC_ACK_INTID_MASK          0x000003ff

#ifdef RT_USING_SMP
#define rt_interrupt_nest rt_cpu_self()->irq_nest
#else
extern volatile rt_uint8_t rt_interrupt_nest;
#endif

extern int system_vectors;

/* exception and interrupt handler table */
struct rt_irq_desc isr_table[MAX_HANDLERS];

rt_ubase_t rt_interrupt_from_thread;
rt_ubase_t rt_interrupt_to_thread;
rt_ubase_t rt_thread_switch_interrupt_flag;

void rt_hw_vector_init(void)
{
    rt_hw_set_current_vbar((rt_ubase_t)&system_vectors);  // cpu_gcc.S
}

/**
 * This function will initialize hardware interrupt
 */
void rt_hw_interrupt_init(void)
{
    rt_uint32_t gic_dist_base   = 0;
    /* initialize ARM GIC */
    gic_dist_base =  platform_get_gic_dist_base();

    arm_gic_dist_init(0, gic_dist_base, 0);
    arm_gic_redist_init(0);
    arm_gic_cpu_init(0);
}

/**
 * This function will install a interrupt service routine to a interrupt.
 * @param vector the interrupt number
 * @param new_handler the interrupt service routine to be installed
 * @param old_handler the old interrupt service routine
 */
rt_isr_handler_t rt_hw_interrupt_install(int vector, rt_isr_handler_t handler,
        void *param, const char *name)
{
    rt_isr_handler_t old_handler = RT_NULL;

    if (vector < MAX_HANDLERS)
    {
        old_handler = isr_table[vector].handler;

        if (handler != RT_NULL)
        {
#ifdef RT_USING_INTERRUPT_INFO
            rt_strncpy(isr_table[vector].name, name, RT_NAME_MAX);
#endif /* RT_USING_INTERRUPT_INFO */
            isr_table[vector].handler = handler;
            isr_table[vector].param = param;
        }
    }

    return old_handler;
}

/**
 * This function will mask a interrupt.
 * @param vector the interrupt number
 */
void rt_hw_interrupt_mask(int vector)
{
    arm_gic_mask(0, vector);
}

/**
 * This function will un-mask a interrupt.
 * @param vector the interrupt number
 */
void rt_hw_interrupt_umask(int vector)
{
    arm_gic_umask(0, vector);
}

/**
 * This function returns the active interrupt number.
 * @param none
 */
int rt_hw_interrupt_get_irq(void)
{
    return (arm_gic_get_active_irq(0) & GIC_ACK_INTID_MASK);
}

/**
 * This function acknowledges the interrupt.
 * @param vector the interrupt number
 */
void rt_hw_interrupt_ack(int vector)
{
    arm_gic_ack(0, vector);
}

/**
 * This function set interrupt CPU targets.
 * @param vector:   the interrupt number
 *        cpu_mask: target cpus mask, one bit for one core
 */
void rt_hw_interrupt_set_target_cpus(int vector, unsigned int cpu_mask)
{
    arm_gic_set_cpu(0, vector, cpu_mask);
}

/**
 * This function get interrupt CPU targets.
 * @param vector: the interrupt number
 * @return target cpus mask, one bit for one core
 */
unsigned int rt_hw_interrupt_get_target_cpus(int vector)
{
    return arm_gic_get_target_cpu(0, vector);
}

/**
 * This function set interrupt triger mode.
 * @param vector: the interrupt number
 *        mode:   interrupt triger mode; 0: level triger, 1: edge triger
 */
void rt_hw_interrupt_set_triger_mode(int vector, unsigned int mode)
{
    arm_gic_set_configuration(0, vector, mode);
}

/**
 * This function get interrupt triger mode.
 * @param vector: the interrupt number
 * @return interrupt triger mode; 0: level triger, 1: edge triger
 */
unsigned int rt_hw_interrupt_get_triger_mode(int vector)
{
    return arm_gic_get_configuration(0, vector);
}

/**
 * This function set interrupt pending flag.
 * @param vector: the interrupt number
 */
void rt_hw_interrupt_set_pending(int vector)
{
    arm_gic_set_pending_irq(0, vector);
}

/**
 * This function get interrupt pending flag.
 * @param vector: the interrupt number
 * @return interrupt pending flag, 0: not pending; 1: pending
 */
unsigned int rt_hw_interrupt_get_pending(int vector)
{
    return arm_gic_get_pending_irq(0, vector);
}

/**
 * This function clear interrupt pending flag.
 * @param vector: the interrupt number
 */
void rt_hw_interrupt_clear_pending(int vector)
{
    arm_gic_clear_pending_irq(0, vector);
}

/**
 * This function set interrupt priority value.
 * @param vector: the interrupt number
 *        priority: the priority of interrupt to set
 */
void rt_hw_interrupt_set_priority(int vector, unsigned int priority)
{
    arm_gic_set_priority(0, vector, priority);
}

/**
 * This function get interrupt priority.
 * @param vector: the interrupt number
 * @return interrupt priority value
 */
unsigned int rt_hw_interrupt_get_priority(int vector)
{
    return arm_gic_get_priority(0, vector);
}

/**
 * This function set priority masking threshold.
 * @param priority: priority masking threshold
 */
void rt_hw_interrupt_set_priority_mask(unsigned int priority)
{
    arm_gic_set_interface_prior_mask(0, priority);
}

/**
 * This function get priority masking threshold.
 * @param none
 * @return priority masking threshold
 */
unsigned int rt_hw_interrupt_get_priority_mask(void)
{
    return arm_gic_get_interface_prior_mask(0);
}

/**
 * This function set priority grouping field split point.
 * @param bits: priority grouping field split point
 * @return 0: success; -1: failed
 */
int rt_hw_interrupt_set_prior_group_bits(unsigned int bits)
{
    int status;

    if (bits < 8)
    {
        arm_gic_set_binary_point(0, (7 - bits));
        status = 0;
    }
    else
    {
        status = -1;
    }

    return (status);
}

/**
 * This function get priority grouping field split point.
 * @param none
 * @return priority grouping field split point
 */
unsigned int rt_hw_interrupt_get_prior_group_bits(void)
{
    unsigned int bp;

    bp = arm_gic_get_binary_point(0) & 0x07;

    return (7 - bp);
}

#ifdef RT_USING_SMP
void rt_hw_ipi_send(int ipi_vector, unsigned int cpu_mask)
{
    arm_gic_send_affinity_sgi(0, ipi_vector, cpu_mask, ROUTED_TO_SPEC);
}

void rt_hw_ipi_handler_install(int ipi_vector, rt_isr_handler_t ipi_isr_handler)
{
    /* note: ipi_vector maybe different with irq_vector */
    rt_hw_interrupt_install(ipi_vector, ipi_isr_handler, 0, "IPI_HANDLER");
}
#endif