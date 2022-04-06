#include <linux/cdev.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kdev_t.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>

MODULE_LICENSE("Dual MIT/GPL");
MODULE_AUTHOR("National Cheng Kung University, Taiwan");
MODULE_DESCRIPTION("Fibonacci engine driver");
MODULE_VERSION("0.1");

#define DEV_FIBONACCI_NAME "fibonacci"

/* MAX_LENGTH is set to 92 because
 * ssize_t can't fit the number > 92
 */
#define MAX_LENGTH 500
#define SIZE 110
#define BNSIZE 100000
#define MAXSLOT 0xffffffffUL + 1UL
typedef struct _bn {
    unsigned int number[BNSIZE];
    unsigned int size;
} bn;
static dev_t fib_dev = 0;
static struct cdev *fib_cdev;
static struct class *fib_class;
static DEFINE_MUTEX(fib_mutex);

bn *init_bn(int num)
{
    bn *n = (bn *) kmalloc(sizeof(bn), GFP_KERNEL);
    for (int i = 0; i < BNSIZE; ++i)
        n->number[i] = 0;
    n->number[0] = num;
    n->size = 1;
    return n;
}

static void add_bn(bn *a, bn *b, bn *c)
{
    unsigned long carry = 0;
    int i;
    c->size = a->size;
    for (i = 0; i < b->size; ++i) {
        unsigned int sum = a->number[i] + b->number[i] + carry;
        carry =
            ((unsigned long) a->number[i] + b->number[i] + carry) >= MAXSLOT;
        c->number[i] = sum;
    }
    for (; i < a->size; ++i) {
        unsigned int sum = a->number[i] + carry;
        carry = ((unsigned long) a->number[i] + carry) >= MAXSLOT;
        c->number[i] = sum;
    }
    if (carry) {
        c->size++;
        c->number[i]++;
    }
}
static void swap_bn(bn **a, bn **b)
{
    bn *tmp = *a;
    *a = *b;
    *b = tmp;
}
static char *bn_to_string(const bn *src)
{
    // log10(x) = log2(x) / log2(10) ~= log2(x) / 3.322
    size_t len = (8 * sizeof(int) * src->size) / 3 + 2;
    char *s = kmalloc(len, GFP_KERNEL);
    char *p = s;
    memset(s, '0', len - 1);
    s[len - 1] = '\0';

    for (int i = src->size - 1; i >= 0; i--) {
        for (unsigned long d = 1U << 31; d; d >>= 1) {
            // double logical not can make the result to 1
            int carry = !!(d & src->number[i]);
            for (int j = len - 2; j >= 0; j--) {
                s[j] += s[j] - '0' + carry;
                carry = (s[j] > '9');
                if (carry)
                    s[j] -= 10;
            }
        }
    }
    // skip leading zero
    while (p[0] == '0' && p[1] != '\0') {
        p++;
    }
    memmove(s, p, strlen(p) + 1);
    return s;
}
static int fibo(int num, char *buf)
{
    if (num == 0) {
        copy_to_user(buf, "0", 2);
        return 0;
    }
    if (num == 1) {
        copy_to_user(buf, "1", 2);
        return 1;
    }
    bn *a = init_bn(1);
    bn *b = init_bn(0);
    bn *c = init_bn(0);
    for (int i = 2; i <= num; ++i) {
        add_bn(a, b, c);
        swap_bn(&a, &c);
        swap_bn(&b, &c);
    }
    char *res = bn_to_string(a);
    copy_to_user(buf, res, strlen(res) + 1);
    kfree(res);
    kfree(a);
    kfree(b);
    kfree(c);
    return num;
}
static int fib_open(struct inode *inode, struct file *file)
{
    if (!mutex_trylock(&fib_mutex)) {
        printk(KERN_ALERT "fibdrv is in use");
        return -EBUSY;
    }
    return 0;
}

static int fib_release(struct inode *inode, struct file *file)
{
    mutex_unlock(&fib_mutex);
    return 0;
}
static ktime_t kt;
static long long fib_time_proxy(long long k, char *buf)
{
    kt = ktime_get();
    long long result = fibo(k, buf);
    kt = ktime_sub(ktime_get(), kt);
    return result;
}
/* calculate the fibonacci number at given offset */
static ssize_t fib_read(struct file *file,
                        char *buf,
                        size_t size,
                        loff_t *offset)
{
    return (ssize_t) fib_time_proxy(*offset, buf);
}
/* write operation is skipped */
static ssize_t fib_write(struct file *file,
                         const char *buf,
                         size_t size,
                         loff_t *offset)
{
    return ktime_to_ns(kt);
}
static loff_t fib_device_lseek(struct file *file, loff_t offset, int orig)
{
    loff_t new_pos = 0;
    switch (orig) {
    case 0: /* SEEK_SET: */
        new_pos = offset;
        break;
    case 1: /* SEEK_CUR: */
        new_pos = file->f_pos + offset;
        break;
    case 2: /* SEEK_END: */
        new_pos = MAX_LENGTH - offset;
        break;
    }

    if (new_pos > MAX_LENGTH)
        new_pos = MAX_LENGTH;  // max case
    if (new_pos < 0)
        new_pos = 0;        // min case
    file->f_pos = new_pos;  // This is what we'll use now
    return new_pos;
}

const struct file_operations fib_fops = {
    .owner = THIS_MODULE,
    .read = fib_read,
    .write = fib_write,
    .open = fib_open,
    .release = fib_release,
    .llseek = fib_device_lseek,
};

static int __init init_fib_dev(void)
{
    int rc = 0;

    mutex_init(&fib_mutex);

    // Let's register the device
    // This will dynamically allocate the major number
    rc = alloc_chrdev_region(&fib_dev, 0, 1, DEV_FIBONACCI_NAME);

    if (rc < 0) {
        printk(KERN_ALERT
               "Failed to register the fibonacci char device. rc = %i",
               rc);
        return rc;
    }

    fib_cdev = cdev_alloc();
    if (fib_cdev == NULL) {
        printk(KERN_ALERT "Failed to alloc cdev");
        rc = -1;
        goto failed_cdev;
    }
    fib_cdev->ops = &fib_fops;
    rc = cdev_add(fib_cdev, fib_dev, 1);

    if (rc < 0) {
        printk(KERN_ALERT "Failed to add cdev");
        rc = -2;
        goto failed_cdev;
    }

    fib_class = class_create(THIS_MODULE, DEV_FIBONACCI_NAME);

    if (!fib_class) {
        printk(KERN_ALERT "Failed to create device class");
        rc = -3;
        goto failed_class_create;
    }

    if (!device_create(fib_class, NULL, fib_dev, NULL, DEV_FIBONACCI_NAME)) {
        printk(KERN_ALERT "Failed to create device");
        rc = -4;
        goto failed_device_create;
    }
    return rc;
failed_device_create:
    class_destroy(fib_class);
failed_class_create:
    cdev_del(fib_cdev);
failed_cdev:
    unregister_chrdev_region(fib_dev, 1);
    return rc;
}

static void __exit exit_fib_dev(void)
{
    mutex_destroy(&fib_mutex);
    device_destroy(fib_class, fib_dev);
    class_destroy(fib_class);
    cdev_del(fib_cdev);
    unregister_chrdev_region(fib_dev, 1);
}

module_init(init_fib_dev);
module_exit(exit_fib_dev);
