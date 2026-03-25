struct flash_identification {
	unsigned flash_id;
	unsigned mask;
	unsigned density;
	unsigned widebus;
	unsigned pagesize;
	unsigned blksize;
	unsigned oobsize;
	char man[32];
};

struct flash_info {
	unsigned id;
	unsigned type;
	unsigned vendor;
	unsigned device;
	unsigned page_size;
	unsigned block_size;
	unsigned spare_size;
	unsigned num_blocks;
	char manufactory[32];
};

static struct flash_identification supported_flash[] =
{
	/*Flash ID		ID 			Mask Density(MB)  	Wid 	Pgsz	Blksz		oobsz	Manuf */
	{0x00000000,	0xFFFFFFFF,	0,					0,		0,		0,			0,		"ONFI"},
	{0x1500aaec,	0xFF00FFFF,	(256<<20),			0,		2048,	(2048<<6),	64,		"Sams"},
	{0x5500baec,	0xFF00FFFF,	(256<<20),			1,		2048,	(2048<<6),	64,		"Sams"},
	{0x1500aa98,	0xFFFFFFFF,	(256<<20),			0,		2048,	(2048<<6),	64,		"Tosh"},
	{0x5500ba98,	0xFFFFFFFF,	(256<<20),			1,		2048,	(2048<<6),	64,		"Tosh"},
	{0xd580b12c,	0xFFFFFFFF,	(256<<20),			1,		2048,	(2048<<6),	64,		"Micr"},
	{0x5590bc2c,	0xFFFFFFFF,	(512<<20),			1,		2048,	(2048<<6),	64,		"Micr"},
	{0x1580aa2c,	0xFFFFFFFF,	(256<<20),			0,		2048,	(2048<<6),	64,		"Micr"},
	{0x1590aa2c,	0xFFFFFFFF,	(256<<20),			0,		2048,	(2048<<6),	64,		"Micr"},
	{0x1590ac2c,	0xFFFFFFFF,	(512<<20),			0,		2048,	(2048<<6),	64,		"Micr"},
	{0x5580baad,	0xFFFFFFFF,	(256<<20),			1,		2048,	(2048<<6),	64,		"Hynx"},
	{0x5510baad,	0xFFFFFFFF,	(256<<20),			1,		2048,	(2048<<6),	64,		"Hynx"},
	{0x6600bcec,	0xFF00FFFF,	(512<<20),			1,		4096,	(4096<<6),	128,	"Sams"},
	{0x0000aaec,	0x0000FFFF,	(256<<20),			1,		2048,	(2048<<6),	64,		"Samsung"},	/*2Gbit*/
	{0x0000acec,	0x0000FFFF,	(512<<20),			1,		2048,	(2048<<6),	64,		"Samsung"},	/*4Gbit*/
	{0x0000bcec,	0x0000FFFF,	(512<<20),			1,		2048,	(2048<<6),	64,		"Samsung"},	/*4Gbit*/
	{0x6601b3ec,	0xFFFFFFFF,	(1024<<20),			1,		4096,	(4096<<6),	128,	"Samsung"},	/*8Gbit 4Kpage*/
	{0x0000b3ec,	0x0000FFFF,	(1024<<20),			1,		2048,	(2048<<6),	64,		"Samsung"},	/*8Gbit*/
	{0x0000ba2c,	0x0000FFFF,	(256<<20),			1,		2048,	(2048<<6),	64,		"Micron"},	/*2Gbit*/
	{0x0000bc2c,	0x0000FFFF,	(512<<20),			1,		2048,	(2048<<6),	64,		"Micron"},	/*4Gbit*/
	{0x0000b32c,	0x0000FFFF,	(1024<<20),			1,		2048,	(2048<<6),	64,		"Micron"},	/*8Gbit*/
	{0x0000baad,	0x0000FFFF,	(256<<20),			1,		2048,	(2048<<6),	64,		"Hynix"},	/*2Gbit*/
	{0x0000bcad,	0x0000FFFF,	(512<<20),			1,		2048,	(2048<<6),	64,		"Hynix"},	/*4Gbit*/
	{0x0000b3ad,	0x0000FFFF,	(1024<<20),			1,		2048,	(2048<<6),	64,		"Hynix"},	/*8Gbit*/

	/* Note: Width flag is 0 for 8 bit Flash and 1 for 16 bit flash	  	*/
	/* Note: The First row will be filled at runtime during ONFI probe	*/

};