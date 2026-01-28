/* sync.h - ho tro vung tranh chap nho cho Arduino (ESP) su dung noInterrupts/interrupts */
#ifndef SYNC_H
#define SYNC_H

#ifdef __cplusplus
extern "C" {
#endif

static inline void sync_lock(void) { noInterrupts(); }
static inline void sync_unlock(void) { interrupts(); }

#ifdef __cplusplus
}
#endif

#endif /* SYNC_H */
