#ifndef __PINS_H_
#define __PINS_H_

#define GPIO(PORT, NUM) (((PORT) - 'A') * GPIO_NUM_PINS + (NUM))
#define GPIO_NUM_PINS   16
#define GPIO2PORT(PIN)  ((PIN) / GPIO_NUM_PINS)
#define GPIO2BIT(PIN)   (1U << GPIO2IDX(PIN))
#define GPIO2IDX(PIN)   ((PIN) % GPIO_NUM_PINS)

#define PA0  GPIO('A', 0)
#define PA1  GPIO('A', 1)
#define PA2  GPIO('A', 2)
#define PA3  GPIO('A', 3)
#define PA4  GPIO('A', 4)
#define PA5  GPIO('A', 5)
#define PA6  GPIO('A', 6)
#define PA7  GPIO('A', 7)
#define PA8  GPIO('A', 8)
#define PA9  GPIO('A', 9)
#define PA10 GPIO('A', 10)
#define PA11 GPIO('A', 11)
#define PA12 GPIO('A', 12)
#define PA13 GPIO('A', 13)
#define PA14 GPIO('A', 14)
#define PA15 GPIO('A', 15)
#define PB0  GPIO('B', 0)
#define PB1  GPIO('B', 1)
#define PB2  GPIO('B', 2)
#define PB3  GPIO('B', 3)
#define PB4  GPIO('B', 4)
#define PB5  GPIO('B', 5)
#define PB6  GPIO('B', 6)
#define PB7  GPIO('B', 7)

#endif /* __PINS_H_ */
