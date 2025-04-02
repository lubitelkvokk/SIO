#include <linux/module.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/kdev_t.h>
#include <linux/fs.h>
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/list.h>
#include <linux/slab.h>

#define EL_BUF_SIZE 100


static dev_t first;
static struct cdev c_dev; 
static struct class *cl;
static int last_digit_count;

struct d_list {
  struct list_head mylist;
  char* str; // str[100]
	int curr_str_size;
};

struct d_list* create_and_append_node(void);
void str_copy(char*, char*, int);
char* get_whole_string_from_list(void);

struct d_list* d_list_head;
static int general_str_len;

static int my_open(struct inode *i, struct file *f)
{
  printk(KERN_INFO "Driver: open()\n");
  return 0;
}

static int my_close(struct inode *i, struct file *f)
{
  printk(KERN_INFO "Driver: close()\n");
  return 0;
}

static int find_number_capacity(int number){
	if (number == 0) return 1;
	int capacity;
	capacity = 0;
	while (number > 0){
		number /= 10;
		capacity++;
	}
	return capacity;
}

static char* get_digit_str_from_number(int number, int number_capacity){
	char* string;
	string = kmalloc(sizeof(char) * (number_capacity + 1), GFP_KERNEL);
	string[number_capacity] = ' ';
	while (number_capacity > 0){
		string[--number_capacity] = (number % 10) + '0';
		number /= 10;
	}
	return string;
}

void str_copy(char* src, char* dst, int len){
	int i;
	for (i = 0; i < len; i++){
		dst[i] = src[i];
	}
}

char* get_whole_string_from_list(){
  struct list_head *pos, *q;
  struct d_list *datastructptr;
  int ind = 0;

  printk(KERN_INFO "<<get_whole_string_from_list>>: general len of string is: %d", general_str_len);
	char* final_string = kmalloc(sizeof(char) * (general_str_len + 2), GFP_KERNEL);

  int digit_capacity;
	list_for_each(pos, &d_list_head->mylist) {
    datastructptr = list_entry(pos, struct d_list, mylist);
		str_copy(datastructptr->str, &final_string[ind], datastructptr->curr_str_size);
    ind += datastructptr->curr_str_size;
		printk(KERN_INFO "<<get_whole_string_from_list>>: intermediate result of ind value: %d", ind);
	} 
  printk(KERN_INFO "<<get_whole_string_from_list>>: len of string is: %d", ind);
	final_string[ind] = '\n';
	final_string[ind + 1] = '\0';
	return final_string;
}

static ssize_t my_read(struct file *f, char __user *buf, size_t len, loff_t *off)
{
	char* whole_string;
	whole_string = get_whole_string_from_list();
	if (!whole_string) {
		printk(KERN_ERR "Error: whole_string is NULL\n");
		return -ENOMEM;
	}
  printk(KERN_INFO "My read %s\n", whole_string);
  printk(KERN_INFO "My read, general_str_len: %d\n", general_str_len);
	
	if (*off >= general_str_len) return 0;

	if (*off + len > general_str_len){
		len = general_str_len - *off;
	}

	copy_to_user(buf, whole_string, len + 2);
	*off += len;

	kfree(whole_string);
	return len + 2;
}

struct d_list* create_and_append_node(){
  struct d_list *new = kmalloc(sizeof(struct d_list), GFP_KERNEL);
  if (!new) {
	  printk(KERN_ERR "Failed to allocate memory for list element in <<add_digit_count_to_list>>");
		return NULL;
  }

	new->str = kmalloc(sizeof(char) * EL_BUF_SIZE, GFP_KERNEL);
	new->curr_str_size = 0;

  if (!d_list_head) {
		printk(KERN_ERR "d_list_head is NULL in add_digit_count_to_list\n");
		kfree(new);
		return NULL;
  }

  list_add(&new->mylist, &d_list_head->mylist); 
	printk(KERN_INFO "<<create_and_append_node>> SUCCESSFULL CREATING\n");
	return new;
}

void add_digit_count_to_list(int d_count){

	printk(KERN_INFO "<<add_digit_count_to_list>> ENTER\n");
	struct d_list *curr;
  curr = list_entry(d_list_head->mylist.next, struct d_list, mylist);

	int number_capacity;
	number_capacity = find_number_capacity(d_count);
  printk(KERN_INFO "<<add_digit_count_to_list>> NUMBER CAPACITY: %d\n", number_capacity);
	char* string = get_digit_str_from_number(d_count, number_capacity);

	int index_within_substring;
	index_within_substring = general_str_len % EL_BUF_SIZE;
	printk(KERN_INFO "<<add_digit_count_to_list>> ENTERING TO CYCLE\n", number_capacity);

	int i;
// INCLUDING SPACE SIGN
	for (i = 0; i <= number_capacity; i++){
		if (index_within_substring % EL_BUF_SIZE == 0 && index_within_substring != 0){
			curr = create_and_append_node();
			if (curr == NULL) {
				printk(KERN_ERR "<<add_digit_count_to_list>> failure attempt to create node\n");
				return;
			}
			index_within_substring = 0;
		}
		curr->str[index_within_substring++] = string[i];
// HERE WE CAN REPLACE INDEX_WITHIN_SUBSTRING TO CURR_STR_SIZE
		curr->curr_str_size++;
	}
	general_str_len += number_capacity + 1;
	printk(KERN_INFO "<<add_digit_count_to_list>> curr->curr_str_size=%d\n", curr->curr_str_size);
	kfree(string);
}

int count_digits(const char* buf, size_t len){
  int i;
  size_t ind = 0;
  int digit_count = 0;
  u8 byte;
  char *kbuf = kmalloc(len, GFP_KERNEL);
  if (!kbuf) return -ENOMEM;

  if (copy_from_user(kbuf, buf, len)) {
	      kfree(kbuf);
	          return -EFAULT;
  }
  
  for (i = 0; i < len; i++) {
      if (kbuf[i] >= '0' && kbuf[i] <= '9') {
	      digit_count++;
      }
  }
  kfree(kbuf);
  return digit_count;
}

static ssize_t my_write(struct file *f, const char __user *buf,  size_t len, loff_t *off)
{
  printk(KERN_INFO "Driver: write()\n");
  int digit_count = count_digits(buf, len);
  if (digit_count >= 0) {
	  printk(KERN_INFO "digit count: %d", digit_count);
	  add_digit_count_to_list(digit_count);
  }
  return len;
}

static struct file_operations mychdev_fops =
{
  .owner = THIS_MODULE,
  .open = my_open,
  .release = my_close,
  .read = my_read,
  .write = my_write
};
 
static int __init ch_drv_init(void)
{
    printk(KERN_INFO "Hello!\n");
    if (alloc_chrdev_region(&first, 0, 1, "ch_dev") < 0)
	  {
			return -1;
	  }
    if ((cl = class_create(THIS_MODULE, "chardrv")) == NULL)
	  {
			unregister_chrdev_region(first, 1);
			return -1;
	  }
    if (device_create(cl, NULL, first, NULL, "var4") == NULL)
	  {
			class_destroy(cl);
			unregister_chrdev_region(first, 1);
			return -1;
	  }
    cdev_init(&c_dev, &mychdev_fops);
    if (cdev_add(&c_dev, first, 1) == -1)
	  {
			device_destroy(cl, first);
			class_destroy(cl);
			unregister_chrdev_region(first, 1);
			return -1;
	  }
    
    d_list_head = kmalloc(sizeof(struct d_list), GFP_KERNEL);
    if (!d_list_head) {
      printk(KERN_ERR "Failed to allocate memory for d_list_head\n");
      return -ENOMEM;
    }
		general_str_len = 0;
//		d_list_head->str = kmalloc(sizeof(char) * EL_BUF_SIZE, GFP_KERNEL);
//		d_list_head->curr_str_size = 0;
    INIT_LIST_HEAD(&d_list_head->mylist);
		create_and_append_node();
    printk(KERN_INFO "List was initialized\n");
    return 0;
}
 
static void __exit ch_drv_exit(void)
{
// TO FREE DIGIT LIST
    if (!d_list_head) return;
    
    struct list_head *pos, *q;
    struct d_list *datastructptr;
		if (!list_empty(&d_list_head->mylist)) {
			list_for_each_safe(pos, q, &d_list_head->mylist) {
				datastructptr = list_entry(pos, struct d_list, mylist);
				printk(KERN_INFO "Deleted element with value: %d\n", datastructptr->curr_str_size);
				list_del(pos);
				kfree(datastructptr->str);
				kfree(datastructptr);
			}
		}
    kfree(d_list_head);

    cdev_del(&c_dev);
    device_destroy(cl, first);
    class_destroy(cl);
    unregister_chrdev_region(first, 1);
    printk(KERN_INFO "Bye!!!\n");
}
 
module_init(ch_drv_init);
module_exit(ch_drv_exit);
 
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Author");
MODULE_DESCRIPTION("The first kernel module");

