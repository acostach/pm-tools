#include <linux/platform_device.h>
#include <linux/module.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/pm.h>
#include <linux/fs.h>
#include <asm/uaccess.h>
#include <linux/kernel.h>
#include <linux/cdev.h>

#define DRIVER_NAME "pm_detect"
#define LOG_INFO KERN_INFO DRIVER_NAME ": "
#define LOG_WARN KERN_ALERT DRIVER_NAME ": "

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Alexandru Costache <alexandru@balena.io>");
MODULE_DESCRIPTION("Fake device driver used for detection of supend/resume events");

static int     dev_open(struct inode *, struct file *);
static int     dev_close(struct inode *, struct file *);
static ssize_t dev_read(struct file *, char *, size_t, loff_t *);
static struct  platform_device *pm_dev = NULL;
static int     open_cnt = 0;
static int     major = -1;
static char    buff[3] = { 0 };
static char    *msg_ptr;
static struct  cdev pm_chardev;
static struct  class *pm_class = NULL;

typedef enum power_state
{
    POWERSTATE_INITIALIZED,
    POWERSTATE_SUSPENDED,
    POWERSTATE_RESUMED
} power_state;

power_state curr_state = POWERSTATE_INITIALIZED;

static int fill_state_buff(char *dest, power_state state)
{
    switch (state)
    {
        case POWERSTATE_INITIALIZED:
            strncpy(buff, "I\0", 2);
            break;
        case POWERSTATE_SUSPENDED:
            strncpy(dest, "S\0", 2);
            break;
        case POWERSTATE_RESUMED:
            strncpy(dest, "R\0", 2);
            break;
        default:
            strncpy(dest, "U\0", 2);
    }
    return 2;
}

static int dev_open(struct inode *inode, struct file *file)
{
    if (open_cnt)
    {
        return -EBUSY;
    }

    open_cnt++;
    try_module_get(THIS_MODULE);

    return 0;
}

static int dev_close(struct inode *inode, struct file *file)
{
    if (1 > open_cnt)
        return -EPERM;

    open_cnt--;
    module_put(THIS_MODULE);

    return 0;
}

static ssize_t dev_read(struct file *flip, char *buffer, size_t len, loff_t *offset)
{
    size_t read = 0;

    fill_state_buff(buff, curr_state);
    msg_ptr = buff;

    while (len && *msg_ptr)
    {
        put_user(*(msg_ptr++), buffer++);
        len--;
        read++;
    }

    curr_state = POWERSTATE_INITIALIZED;

    return read;
}

static int pm_dev_suspend(struct device *dev)
{
    printk(LOG_INFO "- got suspend event\n");
    curr_state = POWERSTATE_SUSPENDED;

    return 0;
}

static int pm_dev_resume(struct device *dev)
{
    printk(LOG_INFO "- got resume event\n");

    curr_state = POWERSTATE_RESUMED;

    return 0;
}

static int pm_dev_probe(struct platform_device *pm_dev)
{
    printk(LOG_INFO "probed\n");

    return 0;
}

static int pm_dev_remove(struct platform_device *pm_dev)
{
    printk(LOG_INFO "removed\n");

    return 0;
}

static struct file_operations f_ops =
{
    .read = dev_read,
    .open = dev_open,
    .release = dev_close
};

static const struct dev_pm_ops pm_ops =
{
    .suspend = pm_dev_suspend,
    .resume = pm_dev_resume
};

static struct platform_driver pm_drv =
{
    .probe = pm_dev_probe,
    .remove = pm_dev_remove,
    .driver = {
        .pm = &pm_ops,
        .name = DRIVER_NAME,
        .owner = THIS_MODULE
    }
};

static void clean_chardev(int done)
{
    if (done)
    {
        device_destroy(pm_class, major);
        cdev_del(&pm_chardev);
    }

    if (pm_class)
    {
        class_destroy(pm_class);
    }

    if (major != -1)
    {
        unregister_chrdev_region(major, 1);
    }
}

static int __init pm_module_init(void)
{
    int err, done = 0;

    if ((err = platform_driver_register(&pm_drv)))
    {
        printk(LOG_WARN "platform_driver_register failed - error: %d\n", err);

        return err;
    }

    pm_dev = platform_device_register_simple(DRIVER_NAME, -1, NULL, 0);

    if (IS_ERR(pm_dev))
    {
        err = PTR_ERR(pm_dev);
        printk(LOG_WARN "platform_device_register_simple failed - error: %d\n", err);
        platform_driver_unregister(&pm_drv);

        return err;
    }

    curr_state = POWERSTATE_INITIALIZED;

    fill_state_buff(buff, curr_state);

    if (alloc_chrdev_region(&major, 0, 1,
                          DRIVER_NAME "_proc") < 0)
        goto error;

    if ((pm_class = class_create(THIS_MODULE,
                          DRIVER_NAME "_sys")) == NULL)
        goto error;

    if (device_create(pm_class, NULL, major,
                          NULL, DRIVER_NAME "_dev") == NULL)
        goto error;

    done = 1;

    cdev_init(&pm_chardev, &f_ops);

    if (cdev_add(&pm_chardev, major, 1) == -1)
        goto error;

    return 0;

error:
    clean_chardev(done);

    return -1;
}

static void __exit pm_module_exit(void)
{
    platform_device_unregister(pm_dev);
    platform_driver_unregister(&pm_drv);
    clean_chardev(1);
}

module_init(pm_module_init);
module_exit(pm_module_exit);
