#ifndef __ARCH_OS_H__
#define __ARCH_OS_H__

/*************************** TIME RELATED ***************************/
#define ARCH_OS_WAIT_FOREVER		(0xFFFFFFFF)
#define ARCH_OS_NO_WAIT 			(0)
typedef void *		 				arch_os_timer_handle_t;
uint32_t arch_os_ms_now(void);
void  arch_os_ms_sleep(uint32_t ms);
uint32_t arch_os_ms_elapsed(uint32_t last_ms);

/************************** THREAD RELATED **************************/
#define ARCH_OS_THREAD_SELF			NULL
typedef void *						arch_os_thread_handle_t;
typedef void *						arch_os_function_return_t;
int arch_os_thread_create(arch_os_thread_handle_t* pthread, const char* name, arch_os_function_return_t (*function)(void*), uint32_t stack_size, void* arg, int priority);
int arch_os_thread_delete(arch_os_thread_handle_t thread);

/************************** MUTEX RELATED **************************/
typedef void *						arch_os_mutex_handle_t;
int arch_os_mutex_create(arch_os_mutex_handle_t* phandle);
int arch_os_mutex_delete(arch_os_mutex_handle_t handle);
int arch_os_mutex_put(arch_os_mutex_handle_t handle);
int arch_os_mutex_get(arch_os_mutex_handle_t handle, uint32_t wait_ms);

/************************** QUEUE RELATED **************************/
typedef void *						arch_os_queue_handle_t;
int arch_os_queue_create(arch_os_queue_handle_t* pqhandle, uint32_t q_len, uint32_t item_size);
int arch_os_queue_delete(arch_os_queue_handle_t qhandle);
int arch_os_queue_send(arch_os_queue_handle_t qhandle, const void *msg, uint32_t wait_ms);
int arch_os_queue_recv(arch_os_queue_handle_t qhandle, void *msg, uint32_t wait_ms);

/************************** HARDWARE RELATED **************************/
uint32_t arch_os_get_free_heap_size();
void arch_os_reboot(void);

#endif /* __ARCH_OS_H__ */
