/* SSID Coder for Subterranean Positioning System (SPS) 
 * Copyright (C) 2015  Blair Wyatt
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
 
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

static const unsigned char tag[] = "SPS:"; 

//SPS SSID data structure 248 bits total
struct sps_data {
    unsigned char c_code[3];   //24 bit
	uint32_t      dev_id;      //24 bit
	int32_t       lat;         //32 bit
	int32_t       lng;         //32 bit
	int32_t       altitude;    //32 bit
	int8_t        tx_pwr;      //8  bit
	bool          alt_map;     //1  bit
	bool          three_d_map; //1  bit
	uint16_t      res;         //16 bit
	uint32_t      app_id;      //24 bit
	int8_t        path_loss;   //2  bit
};

char * encode_ssid(struct sps_data encode_data){

	char *encoded_string = (char *) malloc(sizeof(char) * 31);
	
	//For the string data, we can just read it raw; nothing fancy here.
	//Encoding SSIDs doesn't happen often.
	
	//SPS TAG
	encoded_string[ 0] = tag[0];
	encoded_string[ 1] = tag[1];
	encoded_string[ 2] = tag[2];
	encoded_string[ 3] = tag[3];

	//Country Code
	encoded_string[ 4] = encode_data.c_code[0];
	encoded_string[ 5] = encode_data.c_code[1];
	encoded_string[ 6] = encode_data.c_code[2];
	
	//Device ID
	encoded_string[ 7] = (encode_data.dev_id   >> 16) & 0xFF;
	encoded_string[ 8] = (encode_data.dev_id   >>  8) & 0xFF;
	encoded_string[ 9] = (encode_data.dev_id        ) & 0xFF;
	
	//Latitude
	encoded_string[10] = (encode_data.lat      >> 24) & 0xFF;
	encoded_string[11] = (encode_data.lat      >> 16) & 0xFF;
	encoded_string[12] = (encode_data.lat      >>  8) & 0xFF;
	encoded_string[13] = (encode_data.lat           ) & 0xFF;
	
	//Longitude
	encoded_string[14] = (encode_data.lng      >> 24) & 0xFF;
	encoded_string[15] = (encode_data.lng      >> 16) & 0xFF;
	encoded_string[16] = (encode_data.lng      >>  8) & 0xFF;
	encoded_string[17] = (encode_data.lng           ) & 0xFF;
	
	//Altitude
	encoded_string[18] = (encode_data.altitude >> 24) & 0xFF;
	encoded_string[19] = (encode_data.altitude >> 16) & 0xFF;
	encoded_string[20] = (encode_data.altitude >>  8) & 0xFF;
	encoded_string[21] = (encode_data.altitude      ) & 0xFF;
	
	//Tx Power
	encoded_string[22] = (encode_data.tx_pwr        ) & 0xFF;
	
	//Alternate Mapping and Reserve bits
	encoded_string[23] = (encode_data.alt_map << 7     | 
	                      encode_data.three_d_map << 6 |
						 (encode_data.path_loss << 4) & 0x30 |
	                 ((encode_data.res >> 8) & 0x0F)) & 0xFF;
	encoded_string[24] = (encode_data.res           ) & 0xFF;
	
	//Application ID
	encoded_string[25] = (encode_data.app_id   >> 16) & 0xFF;
	encoded_string[26] = (encode_data.app_id   >>  8) & 0xFF;
	encoded_string[27] = (encode_data.app_id        ) & 0xFF;
	
	//Calculate coding mask, check to see if any chars contain
	//special control characters
	//LF - 0A
    //CR - 0D
    //"  - 22	
	//+  - 2B
	int x;
	uint32_t coding_mask = 0;
	for (x = 0; x <= 27; x++)
	{
		if (encoded_string[x] == 0x0a ||
			encoded_string[x] == 0x0d ||
			encoded_string[x] == 0x22 ||
			encoded_string[x] == 0x2b ) 
		{
			coding_mask = (coding_mask << 1) | 1;
			encoded_string[x] = encoded_string[x] + 1;
		} else {
			coding_mask = (coding_mask << 1) | 0;
		}
		//printf("%x\n",coding_mask);
	}
	//Shift to leave 3 bits for control chars.
	coding_mask = coding_mask << 3;
    uint8_t byte_temp = 0;
	uint32_t bit_mask;
	//Now check the coding mask for any control chars.
	for (x = 2; x >= 0; x--)
	{
	   bit_mask = 0xFF << x*8;
	   byte_temp = ((coding_mask >> x*8) & 0xFF);
	   if (byte_temp == 0x0a ||
	       byte_temp == 0x0d ||
		   byte_temp == 0x22 ||
           byte_temp == 0x2b  ) 
		{
			byte_temp = byte_temp + 1;
			coding_mask = (coding_mask & ~bit_mask) | (byte_temp << x*8);
			coding_mask = coding_mask  | (1 << x);
		} 	
	}
	
	encoded_string[28] = (coding_mask   >> 16) & 0xFF;
	encoded_string[29] = (coding_mask   >> 8 ) & 0xFF;
	encoded_string[30] = (coding_mask        ) & 0xFF;
	
	return encoded_string;
	
};

struct sps_data decode_ssid(unsigned char* str_decode){
	
	//Make string "safe" 
	uint8_t ssid[31] = {}; //SSID can be 32 octets, but we will ignore 
	                             //the last octet as some embedded systems
								 //don't implement it
	memcpy(ssid, str_decode, 31);
	
	struct sps_data decoded_data;
	

	//Check coding bits and reconstruct data
	//we don't have to extract and check the coding mask bits
	//if we work from the right
	int x;
	int y = 0;
	for (x = 30; x >= 23; x--)
	{
		if (((ssid[30] >> y) & 0x1) == 1)
			ssid[x] = ssid[x] - 1;
		y++;
	}
	
	y = 0;
	for (x = 22; x >= 15; x--)
	{
		if (((ssid[29] >> y) & 0x1) == 1)
			ssid[x] = ssid[x] - 1;
		y++;
	}
	
	y = 0;
	for (x = 14; x >= 7; x--)
	{
		if (((ssid[28] >> y) & 0x1) == 1)
			ssid[x] = ssid[x] - 1;
		y++;
	}
	
	//Now we can easily populate the struct

	//Country Code (we can ignore the tag)
	decoded_data.c_code[0] = ssid[4];
	decoded_data.c_code[1] = ssid[5];
	decoded_data.c_code[2] = ssid[6];
	decoded_data.c_code[3] = '\0'; //we need to terminate the string

	//All other segments are pretty straight forward
	decoded_data.dev_id			= ssid[7] << 16 | ssid[8] << 8 | ssid[9];
	decoded_data.lat			= ssid[10] << 24 | ssid[11] << 16 | ssid[12] << 8 | ssid[13];
	decoded_data.lng			= ssid[14] << 24 | ssid[15] << 16 | ssid[16] << 8 | ssid[17];
	decoded_data.altitude		= ssid[18] << 24 | ssid[19] << 16 | ssid[20] << 8 | ssid[21];
	decoded_data.tx_pwr			= ssid[22];
	decoded_data.alt_map		= ((ssid[23] & 0x80) >> 7) & 1;
	decoded_data.three_d_map	= ((ssid[23] & 0x40) >> 6) & 1;
	decoded_data.path_loss   	= (ssid[23] & 0x30) >> 4;
	decoded_data.res			= (ssid[23] & 0x0f << 8) | ssid[24];
	decoded_data.app_id			= ssid[25] << 16 | ssid[26] << 8 | ssid[27];
	
	return decoded_data;
	
};

main (int argc, char *argv[])
{
	unsigned char *encoded_string = "";
 	struct sps_data encode_data; 
    struct sps_data decoded_data; 	
    
	strcpy(encode_data.c_code, "AUS");
	encode_data.dev_id		= 99;
	encode_data.lat			= 991234567;
	encode_data.lng			= 997654321;
	encode_data.altitude	= 123456789;
	encode_data.tx_pwr		= -100;
	encode_data.alt_map		= 1;
	encode_data.three_d_map = 0;
	encode_data.path_loss   = 3;
	encode_data.res			= 7;
	encode_data.app_id		= 32;
	
	printf("Raw Data\n------------\n");
	printf("Country Code : %s\n", encode_data.c_code  );
	printf("Device ID    : %u\n", encode_data.dev_id  );
	printf("Latitude     : %d\n", encode_data.lat     );
	printf("Longitude    : %d\n", encode_data.lng     );
	printf("Altitude     : %d\n", encode_data.altitude);
	printf("Tx Power     : %d\n", encode_data.tx_pwr  );
	printf("Alt Mapping  : %d\n", encode_data.alt_map );
	printf("3D Mapping   : %d\n", encode_data.three_d_map );
	printf("Path Loss    : %d\n", encode_data.path_loss );
	printf("Reserved     : %x\n", encode_data.res     );
	printf("App ID       : %u\n", encode_data.app_id  );
	
	encoded_string = encode_ssid(encode_data);
	printf("\nEncoded SSID\n------------\n\"");
	
	//Print each char as char since we might have null terminators
	int i;
    for(i = 0; i < 31; i++)
        printf("\\x%02x", encoded_string[i]);
		//printf("%02d,", encoded_string[i]);
	printf("\"\n\n");
	for(i = 0; i < 31; i++)
        printf("0x%02x,", encoded_string[i]);
		//printf("%02d,", encoded_string[i]);
	printf("\"\n\n");
	
	decoded_data   = decode_ssid(encoded_string);
	
	printf("Decoded SSID\n------------\n");
	printf("Country Code : %s\n", decoded_data.c_code  );
	printf("Device ID    : %u\n", decoded_data.dev_id  );
	printf("Latitude     : %d\n", decoded_data.lat     );
	printf("Longitude    : %d\n", decoded_data.lng     );
	printf("Altitude     : %d\n", decoded_data.altitude);
	printf("Tx Power     : %d\n", decoded_data.tx_pwr  );
	printf("Alt Mapping  : %d\n", decoded_data.alt_map );
	printf("3D Mapping   : %d\n", encode_data.three_d_map );
	printf("Path Loss    : %d\n", encode_data.path_loss );
	printf("Reserved     : %x\n", decoded_data.res     );
	printf("App ID       : %u\n", decoded_data.app_id  );
	
	return 0;
	
};