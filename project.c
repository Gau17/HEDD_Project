// Includes
#include <linux/gpio.h> 
#include <linux/interrupt.h> 
#include <linux/kernel.h>
#include <linux/module.h> 
#include <linux/printk.h> 
#include <linux/version.h> 
#include <linux/atomic.h> 
#include <linux/cdev.h> 
#include <linux/delay.h> 
#include <linux/device.h> 
#include <linux/fs.h> 
#include <linux/init.h> 
#include <linux/types.h> 
#include <linux/uaccess.h>
#include <linux/io.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/err.h>


MODULE_LICENSE("GPL"); 
MODULE_DESCRIPTION("Project Driver");
MODULE_AUTHOR("Gautam");

#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 10, 0) 
	#define NO_GPIO_REQUEST_ARRAY 
#endif 

#define TIMEOUT_SEC 0
#define TIMEOUT_NSEC  100000
#define SUCCESS 0 
#define DEVICE_NAME "project"
#define MY_DELAY HZ / 5
#define BUF_LEN 80


static int irq = -1;
static int irq1 = -1;
static struct hrtimer etx_hr_timer;
static int major;

static int led1Intensity = 0;
static int led2Intensity = 0;
static int led3Intensity = 0;
static int buttonPressCount = 0;

static uint32_t	*ledBaseAddr = NULL;
static struct kobject *fsObject;
 
enum { 
    CDEV_NOT_USED, 
    CDEV_EXCLUSIVE_OPEN, 
};


static atomic_t already_open = ATOMIC_INIT(CDEV_NOT_USED); 
static int device_open(struct inode *, struct file *); 
static int device_release(struct inode *, struct file *); 
static ssize_t device_read(struct file *, char __user *, size_t, loff_t *); 
static ssize_t device_write(struct file *, const char __user *, size_t, loff_t *);
enum hrtimer_restart timer_callback(struct hrtimer *);

static struct class *cls;

static struct file_operations led_Controller_fops = { 
    .read = device_read, 
    .write = device_write, 
    .open = device_open, 
    .release = device_release, 
}; 
 
//Define push button struct
static struct gpio pushButtons[] = { { 18, GPIOF_IN, "BUTTON" }, { 26, GPIOF_IN, "BUTTON" } };

/*
	Interrupt Service Routine
	Executed whenever button is pushed
*/
static irqreturn_t button_isr(int irq, void *data) 
{
    int button1 = gpio_get_value(pushButtons[0].gpio);
	int button2 = gpio_get_value(pushButtons[1].gpio);
    if (button1 == 1 || button2 == 1) {buttonPressCount++;}

    return IRQ_HANDLED;
} 

/*
	Timer Callback function
*/
enum hrtimer_restart timer_callback(struct hrtimer *timer)
{
    static int kounter = 0;

    // Set Duty cycle for LED1
    {   
	if (kounter < led1Intensity/10)
	{
		writel((1<<17), ledBaseAddr+7);
	}
	else
	{
		writel((1<<17), ledBaseAddr+10);
	}
    }
    // Set Duty cycle for LED2
    {
	if (kounter < led2Intensity/10)
	{
		writel((1<<6), ledBaseAddr+7);
	}
	else
	{
		writel((1<<6), ledBaseAddr+10);
	}
    }
    // Set Duty cycle for LED3
    {   
	if (kounter < led3Intensity/10)
	{
		writel((1<<21), ledBaseAddr+7);
	}
	else
	{
		writel((1<<21), ledBaseAddr+10);
	}
    }

    kounter ++;
    if(kounter == 10)
    {
	kounter = 0;
    }

    hrtimer_forward_now(timer,ktime_set(TIMEOUT_SEC, TIMEOUT_NSEC));
    return HRTIMER_RESTART;
}

/*
	Sysfs file system functions
*/

static ssize_t show_led1(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "Current intensity of Led1 = %d", led1Intensity);
}

static ssize_t store_led1(struct kobject *kobj, struct kobj_attribute *attr, const char *buff, size_t len)
{	
	sscanf(buff, "%d", &led1Intensity);
	return -EINVAL;
}

static ssize_t show_led2(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "Current intensity of Led2 = %d", led2Intensity);
}

static ssize_t store_led2(struct kobject *kobj, struct kobj_attribute *attr, const char *buff, size_t len)
{
	sscanf(buff, "%d", &led2Intensity);
	return -EINVAL;
}

static ssize_t show_led3(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "Current intensity of Led3 = %d", led3Intensity);
}

static ssize_t store_led3(struct kobject *kobj, struct kobj_attribute *attr, const char *buff, size_t len)
{	
	sscanf(buff, "%d", &led3Intensity);
	return -EINVAL;
}

static ssize_t show_clicks(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
	return sprintf(buf, "%d", buttonPressCount);
}

static ssize_t store_clicks(struct kobject *kobj, struct kobj_attribute *attr, const char *buff, size_t len)
{	
	sscanf(buff, "%d", &buttonPressCount);
	return -EINVAL;
}

static struct kobj_attribute led1_attribute = __ATTR(led1, 0660, show_led1, store_led1);
static struct kobj_attribute led2_attribute = __ATTR(led2, 0660, show_led2, store_led2);
static struct kobj_attribute led3_attribute = __ATTR(led3, 0660, show_led3, store_led3);
static struct kobj_attribute clicks_attribute = __ATTR(buttonPressCount, 0660, show_clicks, store_clicks);

// Device file system functions

static int device_open(struct inode *inode, struct file *file) 
{ 
    if (atomic_cmpxchg(&already_open, CDEV_NOT_USED, CDEV_EXCLUSIVE_OPEN)) 
        return -EBUSY; 

    pr_info("Project driver is opened\n"); 
    try_module_get(THIS_MODULE); 
 
    return SUCCESS; 
} 

static int device_release(struct inode *inode, struct file *file) 
{ 
    atomic_set(&already_open, CDEV_NOT_USED); 
 
    module_put(THIS_MODULE); 
    pr_info("Project driver is closed\n");

    return SUCCESS; 
}

static ssize_t device_read(struct file *filp, char __user *buffer, size_t length, loff_t *offset) 
{
	pr_info("Reading through project driver.\n");

	if (*offset == 0)
	{
		sprintf(buffer, "Current intensity of Led1 = %d, Current intensity of Led2 = %d, Current intensity of Led3 = %d", led1Intensity, led2Intensity, led3Intensity);
		*offset = strlen(buffer);
	}
	else
	{
		*offset = 0;
	}

	return *offset;
}

static ssize_t device_write(struct file *filp, const char __user *buff, 
    size_t len, loff_t *off) 
{ 
    pr_info("Writing through project driver.\n");

    //Null terminate the string
    char kernel_buffer[BUF_LEN+1] = {"\0"};

    if(copy_from_user(kernel_buffer, buff, len-1)){return -EFAULT;}
    
    // Setting intensity of LED 1
    if(strcmp(kernel_buffer, "Led1_intensity=0"))
    {
        sscanf("0", "%d", &led1Intensity);
    } 

    else if(strcmp(kernel_buffer, "Led1_intensity=25"))
    {
        sscanf("25", "%d", &led1Intensity);
    } 
    else if(strcmp(kernel_buffer, "Led1_intensity=50"))
    {
        sscanf("50", "%d", &led1Intensity);
    }

    else if(strcmp(kernel_buffer, "Led1_intensity=75"))
    {
        sscanf("75", "%d", &led1Intensity);
    }

    else if(strcmp(kernel_buffer, "Led1_intensity=100"))
    {
        sscanf("100", "%d", &led1Intensity);
    }

    // Setting intensity of LED 2
    else if(strcmp(kernel_buffer, "Led2_intensity=0"))
    {
        sscanf("0", "%d", &led2Intensity);
    }

    else if(strcmp(kernel_buffer, "Led2_intensity=25"))
    {
        sscanf("25", "%d", &led2Intensity);
    }

    else if(strcmp(kernel_buffer, "Led2_intensity=50"))
    {
        sscanf("50", "%d", &led2Intensity);
    }

    else if(strcmp(kernel_buffer, "Led2_intensity=75"))
    {
        sscanf("75", "%d", &led2Intensity);
    }

    else if(strcmp(kernel_buffer, "Led2_intensity=100"))
    {
        sscanf("100", "%d", &led2Intensity);
    }

    // Setting intensity of LED 3
    else if(strcmp(kernel_buffer, "Led3_intensity=0"))
    {
        sscanf("0", "%d", &led3Intensity);
    }

    else if(strcmp(kernel_buffer, "Led3_intensity=25"))
    {
        sscanf("25", "%d", &led3Intensity);
    }

    else if(strcmp(kernel_buffer, "Led3_intensity=50"))
    {
        sscanf("50", "%d", &led3Intensity);
    }

    else if(strcmp(kernel_buffer, "Led3_intensity=75"))
    {
        sscanf("75", "%d", &led3Intensity);
    }

    else if(strcmp(kernel_buffer, "Led3_intensity=100"))
    {
        sscanf("100", "%d", &led3Intensity);
    }

    else if(strcmp(kernel_buffer, "clicks=0"))
    {
        sscanf("0", "%d", &buttonPressCount);
    }

    return len; 
} 

static int __init led_controller_init(void) 
{
	major = register_chrdev(0, DEVICE_NAME, &led_Controller_fops); 
 
	if (major < 0) { 
	pr_alert("Registering led controller device failed with %d\n", major); 
	return major; 
	} 

	pr_info("I was assigned major number %d.\n", major); 
	#if LINUX_VERSION_CODE >= KERNEL_VERSION(6, 4, 0) 
	cls = class_create(DEVICE_NAME); 
	#else 
		cls = class_create(THIS_MODULE, DEVICE_NAME); 
	#endif 

	device_create(cls, NULL, MKDEV(major, 0), NULL, DEVICE_NAME);

	// Initialize push button interrupts
	#ifdef NO_GPIO_REQUEST_ARRAY 
		gpio_request(pushButtons[0].gpio, pushButtons[0].label); 
	#else 
		gpio_request_array(pushButtons, ARRAY_SIZE(pushButtons)); 
	#endif 
	
	irq = gpio_to_irq(pushButtons[0].gpio); 
	if (request_irq(irq, button_isr, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "button", NULL) != 0)
	{
		pr_err("Failed to set ISR!");
		return -1;
	}

	#ifdef NO_GPIO_REQUEST_ARRAY 
		gpio_request(pushButtons[1].gpio, pushButtons[1].label); 
	#else 
		gpio_request_array(pushButtons, ARRAY_SIZE(pushButtons)); 
	#endif 

	irq1 = gpio_to_irq(pushButtons[1].gpio); 
	if (request_irq(irq1, button_isr, IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING, "button", NULL) != 0)
	{
		pr_err("Failed to set ISR!");
		return -1;
	}

	// Initialize hw timer
	ktime_t ktime;
	ktime = ktime_set(TIMEOUT_SEC, TIMEOUT_NSEC);
	hrtimer_init(&etx_hr_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	etx_hr_timer.function = &timer_callback;
	hrtimer_start( &etx_hr_timer, ktime, HRTIMER_MODE_REL);

	// Initialize Sysfs
	uint32_t error = 0;

	fsObject = kobject_create_and_add(DEVICE_NAME, kernel_kobj);
	
	if (!fsObject)
		return -ENOMEM;

	error = sysfs_create_file(fsObject, &led1_attribute.attr);
	if (error)
	{
		kobject_put(fsObject);
		pr_info("failed to create the led1 file ""in /sys/kernel/project\n");
	}

	error = sysfs_create_file(fsObject, &led2_attribute.attr);
	if (error)
	{
		kobject_put(fsObject);
		pr_info("failed to create the led2 file ""in /sys/kernel/project\n");
	}

	error = sysfs_create_file(fsObject, &led3_attribute.attr);
	if (error)
	{
		kobject_put(fsObject);
		pr_info("failed to create the led3 file ""in /sys/kernel/project\n");
	}
	
	error = sysfs_create_file(fsObject, &clicks_attribute.attr);
	if (error)
	{
		kobject_put(fsObject);
		pr_info("failed to create the led3 file ""in /sys/kernel/project\n");
	}

	// Initialize LEDs
	ledBaseAddr = ioremap(0xfe200000, 4*16);
	
	writel((1<<21), ledBaseAddr + 1);	// GPIO17 (LED1)
	writel((1<<18), ledBaseAddr);		// GPIO6 (LED2)
	writel((1<<3), ledBaseAddr + 2);	// GPIO21 (LED3)
	
	pr_info("Project Device created on /dev/%s\n", DEVICE_NAME); 

	return SUCCESS;

} 

static void __exit led_controller_exit(void) 
{ 
	free_irq(irq, NULL); 
	#ifdef NO_GPIO_REQUEST_ARRAY 
		gpio_free(pushButtons[0].gpio); 
	#else  
		gpio_free_array(pushButtons, ARRAY_SIZE(pushButtons)); 
	#endif

	hrtimer_cancel(&etx_hr_timer); 
	iounmap(ledBaseAddr);
	kobject_put(fsObject);

	device_destroy(cls, MKDEV(major, 0)); 
	class_destroy(cls); 
     
	unregister_chrdev(major, DEVICE_NAME);
	pr_info("Project Device Deleted Successfuly");
}
 
module_init(led_controller_init); 
module_exit(led_controller_exit); 
