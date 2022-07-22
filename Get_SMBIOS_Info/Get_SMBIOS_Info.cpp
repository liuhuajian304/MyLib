
#include <iostream>
#include <windows.h>
#include "Winbase.h"
#include <stdio.h>
#include <string>

typedef struct _dmi_header
{
	BYTE type;
	BYTE length;
	WORD handle;
}dmi_header;

typedef struct _RawSMBIOSData
{
	BYTE    Used20CallingMethod;
	BYTE    SMBIOSMajorVersion;
	BYTE    SMBIOSMinorVersion;
	BYTE    DmiRevision;
	DWORD   Length;
	BYTE    SMBIOSTableData[];
}RawSMBIOSData;

const char* dmi_chassis_type(BYTE code)
{
	/* 7.4.1 */
	static const char* type[] = {
		"Other", /* 0x01 */
		"Unknown",
		"Desktop",
		"Low Profile Desktop",
		"Pizza Box",
		"Mini Tower",
		"Tower",
		"Portable",
		"Laptop",
		"Notebook",
		"Hand Held",
		"Docking Station",
		"All In One",
		"Sub Notebook",
		"Space-saving",
		"Lunch Box",
		"Main Server Chassis", /* CIM_Chassis.ChassisPackageType says "Main System Chassis" */
		"Expansion Chassis",
		"Sub Chassis",
		"Bus Expansion Chassis",
		"Peripheral Chassis",
		"RAID Chassis",
		"Rack Mount Chassis",
		"Sealed-case PC",
		"Multi-system",
		"CompactPCI",
		"AdvancedTCA",
		"Blade",
		"Blade Enclosing" /* 0x1D */
	};

	code &= 0x7F; /* bits 6:0 are chassis type, 7th bit is the lock bit */

	if (code >= 0x01 && code <= 0x1D)
		return type[code - 0x01];
	return "unknown";
}

const char* dmi_string(const dmi_header* dm, BYTE s)
{
	char* bp = (char*)dm;
	size_t i, len;

	if (s == 0)
		return "Not Specified";

	bp += dm->length;
	while (s > 1 && *bp)
	{
		bp += strlen(bp);
		bp++;
		s--;
	}

	if (!*bp)
		return "BAD_INDEX";

	/* ASCII filtering */
	len = strlen(bp);
	for (i = 0; i < len; i++)
		if (bp[i] < 32 || bp[i] == 127)
			bp[i] = '.';

	return bp;
}

int GetSmBiosInfo()
{
	int ret = 0;
	RawSMBIOSData* Smbios;
	dmi_header* h = NULL;
	int flag = 1;

	// 第一次获取SMBios缓冲区大小
	ret = GetSystemFirmwareTable('RSMB', 0, 0, 0);
	if (!ret)
	{
		printf("Function failed!\n");
		return 1;
	}

	DWORD bufsize = ret;
	char* buf = new char[bufsize];
	memset(buf, 0, bufsize);

	// 获取SMBios表
	ret = GetSystemFirmwareTable('RSMB', 0, buf, bufsize);
	if (!ret)
	{
		printf("Function failed!\n");
		delete[]buf;
		return 1;
	}

	Smbios = (RawSMBIOSData*)buf;

	BYTE* p = Smbios->SMBIOSTableData;

	if (Smbios->Length != bufsize - 8)
	{
		printf("Smbios length error\n");
		delete[]buf;
		return 1;
	}


	for (int i = 0; i < Smbios->Length; i++)
	{
		h = (dmi_header*)p;

		if (h->type == 0 && flag) //BIOS 信息 (Type 0)
		{
			printf("\nType %02d - [BIOS]\n", h->type);
			printf("\tBIOS Vendor : %s\n", dmi_string(h, p[0x4]));
			printf("\tBIOS Version: %s\n", dmi_string(h, p[0x5]));
			printf("\tRelease Date: %s\n", dmi_string(h, p[0x8]));
			if (p[0x16] != 0xff && p[0x17] != 0xff)
				printf("\tEC version: %d.%d\n", p[0x16], p[0x17]);

			flag = 0;
		}

		else if (h->type == 1) //系统信息 (Type 1)可以获取Bios序列号
		{
			printf("\nType %02d - [System Information]\n", h->type);
			printf("\tManufacturer: %s\n", dmi_string(h, p[0x4]));
			printf("\tProduct Name: %s\n", dmi_string(h, p[0x5]));
			printf("\tVersion: %s\n", dmi_string(h, p[0x6]));
			printf("\tSerial Number: %s\n", dmi_string(h, p[0x7]));
			printf("\tSKU Number: %s\n", dmi_string(h, p[0x19]));
			printf("\tFamily: %s\n", dmi_string(h, p[0x1a]));
		}

		else if (h->type == 2) //基板（或模块）信息 (Type 2)可以获取baseboard序列号
		{
			printf("\nType %02d - [System Information]\n", h->type);
			printf("\tManufacturer: %s\n", dmi_string(h, p[0x4]));
			printf("\tProduct: %s\n", dmi_string(h, p[0x5]));
			printf("\tVersion: %s\n", dmi_string(h, p[0x6]));
			printf("\tSerial Number: %s\n", dmi_string(h, p[0x7]));
			printf("\tAsset Tag: %s\n", dmi_string(h, p[0x8]));
			printf("\tLocation in Chassis: %s\n", dmi_string(h, p[0x0a]));
		}

		else if (h->type == 3) //系统外围或底架 (Type 3)
		{
			printf("\nType %02d - [System Enclosure or Chassis]\n", h->type);
			printf("\tManufacturer: %s\n", dmi_string(h, p[0x04]));
			printf("\tType: %s\n", dmi_chassis_type(p[0x05]));
			printf("\tVersion: %s\n", dmi_string(h, p[0x06]));
			printf("\tSerial Number: %s\n", dmi_string(h, p[0x07]));
			printf("\tAsset Tag Number: %s\n", dmi_string(h, p[0x08]));
			printf("\tVersion: %s\n", dmi_string(h, p[0x10]));
		}

		else if (h->type == 4) //处理器信息 (Type 4) 可以获取CPU序列号
		{
			printf("\nType %02d - [Processor Information]\n", h->type);
			printf("\tSocket Designation: %s\n", dmi_string(h, p[0x04]));
			printf("\tProcessor Manufacturer: %s\n", dmi_string(h, p[0x07]));
			printf("\tProcessor Version: %s\n", dmi_string(h, p[0x10]));
			printf("\tVoltage: %d (Bit0 - 5v, Bit1 - 3.3v, Bit2 - 2.9v)\n", p[0x11] & 0x7);

			printf("\tExternal Clock: %d MHz\n", p[0x12] + p[0x13] * 0x100);
			printf("\tMax Speed: %d MHz\n", p[0x14] + p[0x15] * 0x100);
			printf("\tCurrent Speed: %d MHz\n", p[0x16] + p[0x17] * 0x100);
			printf("\tSerial Number: %s\n", dmi_string(h, p[0x20]));
			printf("\tAsset Tag: %s\n", dmi_string(h, p[0x21]));
			printf("\tPart Number: %s\n", dmi_string(h, p[0x22]));
		}

		else if (h->type == 17) //存储设备(Type 17)可以获取内存序列号
		{
			if (p[0xc] + p[0xd] * 0x100 == 0)
				continue;
			printf("\nType %02d - [Memory]\n", h->type);
			printf("\tTotal Width: %d\n", p[0x8]);
			printf("\tData Width:%d\n", p[0xa]);
			printf("\tSize: %d MB\n", p[0xc] + p[0xd] * 0x100);
			printf("\tSpeed: %dMHz\n", p[0x15] + p[0x16] * 0x100);
			printf("\tBank Locator: %s\n", dmi_string(h, p[0x11]));
			printf("\tManufacturer: %s\n", dmi_string(h, p[0x17]));
			printf("\tSerial Number: %s\n", dmi_string(h, p[0x18]));
			printf("\tAsset Tag: %s\n", dmi_string(h, p[0x19]));
			printf("\tPart Number: %s\n", dmi_string(h, p[0x1A]));
		}

		p += h->length;
		while ((*(WORD*)p) != 0) p++;
		p += 2;
	}

	//getchar();
	delete[]buf;
	return 0;
}


/*
	SMBIOS信息概述 – DMI   参考链接：https://blog.csdn.net/u010479322/article/details/72719715
*/
int main()
{
	printf("开始获取SmBios信息：\n");
	printf("----------------------------------------------");
	GetSmBiosInfo();
	printf("----------------------------------------------\n");
	system("pause");
	return 0;
}

