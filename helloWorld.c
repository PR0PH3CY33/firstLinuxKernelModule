#include <linux/init.h>

#include <linux/module.h>

#include <linux/fs.h>

#include <linux/cdev.h>

#include <linux/semaphore.h>

#include <asm/uaccess.h>




//run sudo mknod /dev/{DEVICE_NAME} c {Major Number} 0



MODULE_LICENSE("Dual BSD/GPL");

MODULE_AUTHOR("Antoine Zayyat");


#define DEVICE_NAME "cdriver"



dev_t devNum;

int result;

struct cdev *myCDev;

struct file_operations fOps;

int majorNumber;



struct charDriver {

	char data[4];

	struct semaphore sem;

} charDevice;




int deviceOpen(struct inode *inode, struct file *filp) {

	if(down_interruptible(&charDevice.sem) != 0) {

		return -1; //Couldn't Lock Device During Open

	}

	return 0;

}


ssize_t deviceRead(struct file *filp, char *buffer, size_t bufferSize, loff_t *currentOffset) {

	result = copy_to_user(buffer, charDevice.data, bufferSize);

	return result;


}



ssize_t deviceWrite(struct file *filp, const char *buffer, size_t bufferSize, loff_t *currentOffset) {

        result = copy_from_user(charDevice.data, buffer, bufferSize);

	printk(KERN_ALERT "%s\n", charDevice.data);

        return result;


}


int deviceRelease(struct inode *inode, struct file *filp) {

	filp_close(filp, NULL);

	up(&charDevice.sem);

	return 0;

}




struct file_operations fOps = {

	.owner = THIS_MODULE,

	.open = deviceOpen,

	.release = deviceRelease,

	.read = deviceRead,

	.write = deviceWrite

};


static int deviceInitialization(void) {

	result = alloc_chrdev_region(&devNum, 0, 1, DEVICE_NAME);

	if(result < 0) {

		printk(KERN_ALERT "Couldn't Allocate A Major Number");

		return result;

	}

	majorNumber = MAJOR(devNum);

	printk(KERN_INFO "%d\n", majorNumber);

	//------------------------//

	myCDev = cdev_alloc();

	myCDev->ops = &fOps;

	myCDev->owner = THIS_MODULE;

	result = cdev_add(myCDev, devNum, 1);

	if(result < 0) {

		printk(KERN_ALERT "Unable To Add CDEV Into The Kernel");

		return result;

	}

	sema_init(&charDevice.sem, 1);

	return 0;

}



static int hello_init(void) {

	deviceInitialization();

	return 0;

}


static void hello_exit(void) {

	cdev_del(myCDev);

	unregister_chrdev_region(devNum, 1);


}



module_init(hello_init);

module_exit(hello_exit);
