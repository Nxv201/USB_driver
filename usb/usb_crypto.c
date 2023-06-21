#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/ctype.h>
#include <linux/crypto.h>
#include <linux/usb.h>

#define MEM_SIZE 1024

uint8_t *kernel_buffer;

struct crypto_cipher *tfm;
char key[20] = "01234567";
char type[100];
char data[MEM_SIZE];
size_t data_len = 0;

static struct usb_device *device;
static struct usb_class_driver class;


int string2hex(char *in, int len, char *out)
{
    int i;

    for (i = 0; i < len; i++)
    {
        sprintf(out, "%s%02hhx", out, in[i]);
    }
    return 0;
}

int hex2string(char *in, int len, char *out)
{
    int i;
    int converter[105];
    converter['0'] = 0;
    converter['1'] = 1;
    converter['2'] = 2;
    converter['3'] = 3;
    converter['4'] = 4;
    converter['5'] = 5;
    converter['6'] = 6;
    converter['7'] = 7;
    converter['8'] = 8;
    converter['9'] = 9;
    converter['a'] = 10;
    converter['b'] = 11;
    converter['c'] = 12;
    converter['d'] = 13;
    converter['e'] = 14;
    converter['f'] = 15;

    for (i = 0; i < len; i = i + 2)
    {
        char byte = converter[(int)in[i]] << 4 | converter[(int)in[i + 1]];
        out[i / 2] = byte;
    }

    return 0;
}





static int pen_open(struct inode *i, struct file *f)
{
    return 0;
}
static int pen_close(struct inode *i, struct file *f)
{
    return 0;
}
static ssize_t pen_read(struct file *file, char *user_buf, size_t len, loff_t *off)
{
    char cipher[1000];
    char hex_cipher[1000];
    int i, j;

    printk("data_len: %ld\n", data_len);

    memset(cipher, 0, sizeof(cipher));
    memset(hex_cipher, 0, sizeof(hex_cipher));

    for (i = 0; i < data_len / 8; i++)
    {
        char one_data[20], one_cipher[20];

        memset(one_data, 0, sizeof(one_data));
        memset(one_cipher, 0, sizeof(one_cipher));

        for (j = 0; j < 8; j++)
            one_data[j] = data[i * 8 + j];

        printk("one data: %s\n", one_data);

        if (strcmp(type, "encrypt") == 0)
            crypto_cipher_encrypt_one(tfm, one_cipher, one_data);
        if (strcmp(type, "decrypt") == 0)
            crypto_cipher_decrypt_one(tfm, one_cipher, one_data);
        for (j = 0; j < 8; j++)
            cipher[i * 8 + j] = one_cipher[j];

        printk("one cipher: %s\n", one_cipher);
    }

    string2hex(cipher, data_len, hex_cipher);
    printk("hex cipher: %s\n", hex_cipher);
    copy_to_user(user_buf, hex_cipher, data_len * 2 + 1);

    return 0;
}
static ssize_t pen_write(struct file *file, const char *user_buff, size_t len, loff_t *off)
{
    char buffer[1000], hex_data[1000];
    int i, j;

    memset(buffer, 0, sizeof(buffer));
    memset(data, 0, sizeof(data));
    memset(type, 0, sizeof(type));
    memset(hex_data, 0, sizeof(hex_data));

    copy_from_user(buffer, user_buff, len);

    i = 0;
    j = 0;
    while (buffer[i] != '\n' && j < len)
    {
        type[i] = buffer[j];
        i++;
        j++;
    }

    i = 0;
    j++;
    while (j < len)
    {
        hex_data[i] = buffer[j];
        i++;
        j++;
    }

    printk("type: %s\n", type);
    printk("hex_data: %s\n", hex_data);

    memset(buffer, 0, sizeof(buffer));
    hex2string(hex_data, strlen(hex_data), data);
    printk("data: %s\n", data);

    if (strlen(hex_data) % 16 == 0)
        data_len = ((uint16_t)(strlen(hex_data) / 16)) * 8;
    else
        data_len = ((uint16_t)((strlen(hex_data) / 16) + 1)) * 8;
    return 0;
}

static struct file_operations fops =
{
    .owner = THIS_MODULE,
    .open = pen_open,
    .release = pen_close,
    .read = pen_read,
    .write = pen_write,
};

static int pen_probe(struct usb_interface *interface, const struct usb_device_id *id)
{
    int ret;

    device = interface_to_usbdev(interface);

    class.name = "usb/crypto";
    class.fops = &fops;
    if ((ret = usb_register_dev(interface, &class)) < 0)
    {
        /* Something prevented us from registering this driver */
        printk(KERN_ERR "Not able to get a minor for this device.");
    }
    else
    {
        printk(KERN_INFO "Minor obtained: %d\n", interface->minor);
    }

	tfm = crypto_alloc_cipher("des", 0, 0);
    ret = crypto_cipher_setkey(tfm, key, 8);
	if (ret < 0)
		printk("setting key error.");
	else
		printk("Setting of the key was successful");


    return ret;
}

static void pen_disconnect(struct usb_interface *interface)
{
	crypto_free_cipher(tfm);
    usb_deregister_dev(interface, &class);
}

/* Table of devices that work with this driver */
static struct usb_device_id pen_table[] =
{
    { USB_DEVICE(0x0483, 0x3748) },
    {}
};
MODULE_DEVICE_TABLE (usb, pen_table);

static struct usb_driver pen_driver =
{
    .name = "pen_driver",
    .probe = pen_probe,
    .disconnect = pen_disconnect,
    .id_table = pen_table,
};

static int __init pen_init(void)
{
    int result;

    /* Register this driver with the USB subsystem */
    if ((result = usb_register(&pen_driver)))
    {
        printk(KERN_ERR "usb_register failed. Error number %d", result);
    }
    return result;
}

static void __exit pen_exit(void)
{
    /* Deregister this driver with the USB subsystem */
    usb_deregister(&pen_driver);
}

module_init(pen_init);
module_exit(pen_exit);

MODULE_LICENSE("GPL");

