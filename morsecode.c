#include <linux/module.h>
#include <linux/miscdevice.h>
#include <linux/fs.h>
#include <linux/delay.h>
#include <linux/ctype.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/kfifo.h>

#define MY_DEVICE_FILE  "morse-code"

#define FIFO_SIZE 256

static DECLARE_KFIFO(queue, char, FIFO_SIZE);

/******************************************************
 * LED
 ******************************************************/
#include <linux/leds.h>

DEFINE_LED_TRIGGER(morse_code_trigger);

#define LED_DOT_TIME_ms 200
#define LED_DASH_TIME_ms 600

static void flashChar(int code);

static void led_blink_dot(void)
{
	led_trigger_event(morse_code_trigger, LED_FULL);
	msleep(LED_DOT_TIME_ms);
	led_trigger_event(morse_code_trigger, LED_OFF);
	// msleep(LED_DOT_TIME_ms);
}

static void led_blink_dash(void)
{
	led_trigger_event(morse_code_trigger, LED_FULL);
	msleep(LED_DASH_TIME_ms);
	led_trigger_event(morse_code_trigger, LED_OFF);
	// msleep(LED_DOT_TIME_ms);
}

static void blink_led(char* character)
{
	if(strcmp(character, "1") == 0){
		led_blink_dot();
		kfifo_put(&queue, '.');
	}

	if(strcmp(character, "111") == 0){
		led_blink_dash();
		kfifo_put(&queue, '-');
	}
}

static void led_register(void)
{
	// Setup the trigger's name:
	led_trigger_register_simple("morse-code", &morse_code_trigger);
}

static void led_unregister(void)
{
	// Cleanup
	led_trigger_unregister_simple(morse_code_trigger);
}

/******************************************************
 * Morse Encoding / Decoding
 ******************************************************/
static unsigned short morsecode_codes[] = {
		0xB800,	// A 1011 1
		0xEA80,	// B 1110 1010 1
		0xEBA0,	// C 1110 1011 101
		0xEA00,	// D 1110 101
		0x8000,	// E 1
		0xAE80,	// F 1010 1110 1
		0xEE80,	// G 1110 1110 1
		0xAA00,	// H 1010 101
		0xA000,	// I 101
		0xBBB8,	// J 1011 1011 1011 1
		0xEB80,	// K 1110 1011 1
		0xBA80,	// L 1011 1010 1
		0xEE00,	// M 1110 111
		0xE800,	// N 1110 1
		0xEEE0,	// O 1110 1110 111
		0xBBA0,	// P 1011 1011 101
		0xEEB8,	// Q 1110 1110 1011 1
		0xBA00,	// R 1011 101
		0xA800,	// S 1010 1
		0xE000,	// T 111
		0xAE00,	// U 1010 111
		0xAB80,	// V 1010 1011 1
		0xBB80,	// W 1011 1011 1
		0xEAE0,	// X 1110 1010 111
		0xEBB8,	// Y 1110 1011 1011 1
		0xEEA0	// Z 1110 1110 101
};

//This function is from:
//https://stackoverflow.com/questions/16790227/replace-multiple-spaces-by-single-space-in-c
void replace_multi_space_with_single_space(char *str)
{
    char *dest = str;  /* Destination to copy to */

    /* While we're not at the end of the string, loop... */
    while (*str != '\0')
    {
        /* Loop while the current character is a space, AND the next
         * character is a space
         */
        while (*str == ' ' && *(str + 1) == ' ')
            str++;  /* Just skip to next character */

       /* Copy from the "source" string to the "destination" string,
        * while advancing to the next character in both
        */
       *dest++ = *str++;
    }

    /* Make sure the string is properly terminated */    
    *dest = '\0';
}

static int my_open(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "morsecode_driver: In my_open()\n");
	return 0;  // Success
}

static int my_close(struct inode *inode, struct file *file)
{
	printk(KERN_INFO "morsecode_driver: In my_close()\n");
	return 0;  // Success
}

static ssize_t my_read(struct file *file,
		char *buff,	size_t count, loff_t *ppos)
{
	int buffer_idx = 0;
	int bytes_read = 0;
	char i = '0';
	char* string;

	string = kmalloc(kfifo_len(&queue) + 1, GFP_KERNEL);

	// Fill buffer 1 byte at a time
	while (kfifo_get(&queue, &i)){
		string[buffer_idx] = i;
		buffer_idx++;
	}

	//puts a newline at the end of the string for terminal output
	string[buffer_idx] = '\n';

	// if '\n' is the only character
	if(strlen(string) == 1){
		return bytes_read;
	}

		// Fill buffer 1 byte at a time
	for (buffer_idx = 0; buffer_idx < strlen(string); buffer_idx++) {

		// Copy next byte of data into user's buffer.
		// copy_to_user returns 0 for success (# of bytes not copied)
		if (copy_to_user(&buff[buffer_idx], &string[buffer_idx], 1)) {
			printk(KERN_ERR "Unable to write to buffer.");
			return -EFAULT;
		}

		bytes_read++;
	}

	printk(KERN_INFO "morsecode_driver: In my_read()\n");


	// Write to in/out parameters and return:
	*ppos = buffer_idx;
	return bytes_read;  // # bytes actually read.
}

static ssize_t my_write(struct file *file,
		const char *buff, size_t count, loff_t *ppos)
{
	int buff_idx = 0;
	int str_idx = 0;
	char* string;
	int codeToFlash = 0;

	string = kmalloc(count + 1, GFP_KERNEL);

	printk(KERN_INFO "morsecode_driver: In my_write()\n");

	for (buff_idx = 0; buff_idx < count; buff_idx++) {
		char ch;
		// Get the character
		if (copy_from_user(&ch, &buff[buff_idx], sizeof(ch))) {
			return -EFAULT;
		}

		// Skip special characters:
		if (!isalpha(ch) && !isspace(ch)) {
			continue;
		}

		// append character to string
		string[str_idx] = ch;
		str_idx++;
	}

	//preprocessing to trim trailing whitespace
	for(str_idx -= 1; str_idx > 0; str_idx--){
		if(string[str_idx] == ' ' || string[str_idx] == '\n' || string[str_idx] == '\r' || string[str_idx] == '\t'){
			string[str_idx] = '\0';
		}
		else{
			break;
		}
	}

	//preprocessing to skip leading whitespace
	buff_idx = 0;
	while(string[buff_idx] == ' ' || string[buff_idx] == '\n' || string[buff_idx] == '\r' || string[buff_idx] == '\t'){
		buff_idx++;
	}

	//preprocessing to remove multiple spaces in the middle
	replace_multi_space_with_single_space(string);

	printk(KERN_INFO "resulting string = \"%s\"\n", string);

	for( ; buff_idx < strlen(string); buff_idx++){

		int alphabetIndex = 0;
		char ch;

		ch = string[buff_idx];

		if(ch == ' '){
			kfifo_put(&queue, ' ');
			kfifo_put(&queue, ' ');
			kfifo_put(&queue, ' ');
			msleep(LED_DOT_TIME_ms*3); 
			// the other 4 dot-times
			//come from the led blink end dot-time + the 3 dot-time between letters 
			continue;
		}
		
		alphabetIndex = tolower(ch) - 'a'; //get index for morsecode array

		printk("alphaidx = %d\n", alphabetIndex);

		if(alphabetIndex < 0 || alphabetIndex > 23){
			return count;
		}

		codeToFlash = morsecode_codes[alphabetIndex];

		flashChar(codeToFlash); 

		// count-2 to account for the '\0'
		if(buff_idx == count-2){
			break;
		}

		msleep(LED_DASH_TIME_ms);
	}

	// Return # bytes actually written.
	*ppos += count;
	return count;

}

static void flashChar(int code)
{
	char* string; 
	char* found;
	int count = 0;
	int endOfChar = 0;

	//Convert the hexadecimal value into a string of 1's and 0's
	//This section is from: 
	//https://stackoverflow.com/questions/70068903/unsigned-short-int-to-binary-in-c-without-using-malloc-function
	char* binary = (char*)kmalloc(sizeof(char) * 16, GFP_KERNEL);
	int j = 0;
	unsigned i;
	for (i = 1 << 16; i > 0; i = i / 2) {
		binary[j++] = (code & i) ? '1' : '0'; 
	}
    binary[j]='\0';
	/////////////////////////////////////////////////

	string = kmalloc(sizeof(binary), GFP_KERNEL);

	strcpy(string, binary);

	printk(KERN_INFO "Flashing a character\n");

	// Find last dot or dash of the character
	endOfChar = strlen(string)-1;
	while(string[endOfChar] != '1'){
		endOfChar--;
	}

	//Split the string by 0's so we get "1" or "111" only
	//While loop is from:
	//https://c-for-dummies.com/blog/?p=1769
    while((found = strsep(&string,"0")) != NULL){
		if(strcmp(found, "\0") == 0){
			count++;
			continue;
		}

		blink_led(found);
		count++;
		
		//This avoids needlessly sleeping at the end of a character
		if(count == endOfChar+1){
			break;
		}

		msleep(LED_DOT_TIME_ms);		
	}

	// Places a space between each dot or dash in a character
	kfifo_put(&queue, ' ');
}

static long my_unlocked_ioctl (struct file *file, unsigned int cmd, unsigned long arg)
{
	printk(KERN_INFO "morsecode_driver: In my_unlocked_ioctl()\n");
	return 0;  // Success
}

// Callbacks:  (structure defined in /linux/fs.h)
struct file_operations my_fops = {
	.owner    =  THIS_MODULE,
	.open     =  my_open,
	.release  =  my_close,
	.read     =  my_read,
	.write    =  my_write,
	.unlocked_ioctl =  my_unlocked_ioctl
};

// Character Device info for the Kernel:
static struct miscdevice my_miscdevice = {
		.minor    = MISC_DYNAMIC_MINOR,         // Let the system assign one.
		.name     = MY_DEVICE_FILE,             // /dev/.... file.
		.fops     = &my_fops                    // Callback functions.
};

static int __init morsecode_init(void)
{
    int ret;
	printk(KERN_INFO "----> morsecode driver init(): file /dev/%s.\n", MY_DEVICE_FILE);
	// Register as a misc driver:
	ret = misc_register(&my_miscdevice);

	INIT_KFIFO(queue);

	led_register();

	return ret;
}
static void __exit morsecode_exit(void) 
{
    printk(KERN_INFO "<---- morsecode driver exit().\n");

	// Unregister misc driver
	misc_deregister(&my_miscdevice);
	
	led_unregister();
}
// Link our init/exit functions into the kernel's code.
module_init(morsecode_init);
module_exit(morsecode_exit);
// Information about this module:
MODULE_AUTHOR("Daniel Olivera");
MODULE_DESCRIPTION("A Morsecode driver");
MODULE_LICENSE("GPL"); // Important to leave as GPL.