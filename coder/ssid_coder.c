/* SSID Coder for SubPos
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
 *
 * Compile with gcc -o coder ssid_coder.c
 */
 
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>

//The tag is always valid SSID charset
static const unsigned char tag[] = "SPS"; 

//SPS SSID data structure 248 bits total
struct sps_data {
	uint32_t      dev_id;      //24 bit
	int32_t       lat;         //32 bit
	int32_t       lng;         //32 bit
	int32_t       altitude;    //26 bit (with extra sign bit)
	int16_t       tx_pwr;      //11 bit
	bool          off_map;     //1  bit
	bool          three_d_map; //1  bit
	uint16_t      res;         //12 bit
	uint32_t      app_id;      //24 bit
	uint8_t       path_loss;   //3  bit
};

char * encode_ssid(struct sps_data encode_data){

	char *encoded_string = (char *) malloc(sizeof(char) * 31);
	
	//For the string data, we can just read it raw; nothing fancy here.
	//Encoding SSIDs doesn't happen often.
	
	//SPS TAG
	encoded_string[ 0] = tag[0];
	encoded_string[ 1] = tag[1];
	encoded_string[ 2] = tag[2];
	
	//Device ID
	encoded_string[ 3] = (encode_data.dev_id        >> 16) & 0xFF;
	encoded_string[ 4] = (encode_data.dev_id        >>  8) & 0xFF;
	encoded_string[ 5] = (encode_data.dev_id             ) & 0xFF;
	
	//Latitude
	encoded_string[ 6] = (encode_data.lat           >> 24) & 0xFF;
	encoded_string[ 7] = (encode_data.lat           >> 16) & 0xFF;
	encoded_string[ 8] = (encode_data.lat           >>  8) & 0xFF;
	encoded_string[ 9] = (encode_data.lat                ) & 0xFF;
	
	//Longitude
	encoded_string[10] = (encode_data.lng           >> 24) & 0xFF;
	encoded_string[11] = (encode_data.lng           >> 16) & 0xFF;
	encoded_string[12] = (encode_data.lng           >>  8) & 0xFF;
	encoded_string[13] = (encode_data.lng                ) & 0xFF;
    
    //Application ID
	encoded_string[14] = (encode_data.app_id        >> 16) & 0xFF;
	encoded_string[15] = (encode_data.app_id        >>  8) & 0xFF;
	encoded_string[16] = (encode_data.app_id             ) & 0xFF;
	
	//Altitude
	encoded_string[17] = (abs(encode_data.altitude) >> 18) & 0xFF;
	encoded_string[18] = (abs(encode_data.altitude) >> 10) & 0xFF;
	encoded_string[19] = (abs(encode_data.altitude) >>  2) & 0xFF;
	
	encoded_string[20] =((abs(encode_data.altitude)        & 0x03) << 6)
                       | (encode_data.altitude < 0) << 5
                       |  encode_data.off_map       << 4 
	                   |  encode_data.three_d_map   << 3 
                       |((encode_data.tx_pwr + 1000)>> 8   & 0x07);
    encoded_string[21] = (encode_data.tx_pwr + 1000)       & 0xFF;
	

	encoded_string[22] = (encode_data.path_loss     << 5)  & 0xE0
	                   |((encode_data.res >> 8) & 0x1F);
	encoded_string[23] = (encode_data.res               )  & 0xFF;
	

	
    //Transform encoded string and create "ASCII" 7bit mask
    int x;
    uint32_t ascii_mask = 0; //21 bits (for 21 bytes) to encode
    for (x = 3; x <= 23; x++)
	{
        //check if MSB of byte is 1
        if ((encoded_string[x] & 0x80) == 0x80) {
            encoded_string[x] = encoded_string[x] & 0x7F;
            ascii_mask = (ascii_mask << 1) | 1;
        } else {
            ascii_mask = (ascii_mask << 1) | 0;
        }
        //printf("%x\n",ascii_mask);
	}
    
    //create mask in such a way that we don't have to mask the mask:
    //7 bit coding since we have a nice factor of 7 for num of bytes
    
    encoded_string[24] = (ascii_mask   >> 14) & 0x7F;
    encoded_string[25] = (ascii_mask   >>  7) & 0x7F;
    encoded_string[26] = (ascii_mask        ) & 0x7F;
    
	//Calculate coding mask, check to see if any chars contain
	//special control characters
	//LF - 0A
    //CR - 0D
    //"  - 22	
	//+  - 2B
	//nul- 00
    //sp - 20
    
    //Note 0x7f can never be encoded out if 0x00 is encoded out.

	uint32_t coding_mask = 0;
	for (x = 3; x <= 26; x++)
	{
		if (encoded_string[x] == 0x0a ||
			encoded_string[x] == 0x0d ||
			encoded_string[x] == 0x22 ||
			encoded_string[x] == 0x2b ||
			encoded_string[x] == 0x00 ||
			encoded_string[x] == 0x20 ) 
		{
			coding_mask = (coding_mask << 1) | 1;
			encoded_string[x] = encoded_string[x] + 1;
		} else {
			coding_mask = (coding_mask << 1) | 0;
		}
		//printf("%x\n",coding_mask);
	}
	//Shift to leave 4 bits to encode out control chars from coding mask .
	coding_mask = coding_mask << 4;
    uint8_t byte_temp = 0;
	uint32_t bit_mask;
	//Now check the coding mask for any control chars. Treat as 7 bit word
	for (x = 3; x >= 0; x--)
	{
	   bit_mask = 0x7F << x*7;
	   byte_temp = ((coding_mask >> x*7) & 0x7F);
	   if (byte_temp == 0x0a ||
	       byte_temp == 0x0d ||
		   byte_temp == 0x22 ||
           byte_temp == 0x2b ||
           byte_temp == 0x00 ||
           byte_temp == 0x20 ) 
		{
			byte_temp = byte_temp + 1;
			coding_mask = (coding_mask & ~bit_mask) | (byte_temp << x*7);
			coding_mask = coding_mask  | (1 << x);
		} 	
	}
	
    //Encode as 7 bits x4 (28 bytes being masked; neat factor, no wastage, except for the 4 bits which we can't use)
    encoded_string[27] = (coding_mask   >> 21) & 0x7F;
	encoded_string[28] = (coding_mask   >> 14) & 0x7F;
	encoded_string[29] = (coding_mask   >> 7 ) & 0x7F;
	encoded_string[30] = (coding_mask        ) & 0x7F;
   
	
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

	for (x = 30; x >= 24; x--)
	{
		if (((ssid[30] >> y) & 0x1) == 1)
			ssid[x] = ssid[x] - 1;
		y++;
	}
    
    y = 0;
	for (x = 23; x >= 17; x--)
	{
		if (((ssid[29] >> y) & 0x1) == 1)
			ssid[x] = ssid[x] - 1;
		y++;
	}
	
	y = 0;
	for (x = 16; x >= 10; x--)
	{
		if (((ssid[28] >> y) & 0x1) == 1)
			ssid[x] = ssid[x] - 1;
		y++;
	}
	
	y = 0;
	for (x = 9; x >= 3; x--)
	{
		if (((ssid[27] >> y) & 0x1) == 1)
			ssid[x] = ssid[x] - 1;
		y++;
	}
    
    //Now pull out the "ASCII" mask
    y = 0;
	for (x = 23; x >= 17; x--)
	{
		if (((ssid[26] >> y) & 0x1) == 1)
			ssid[x] = ssid[x] | 0x80;
		y++;
	}
    
    y = 0;
	for (x = 16; x >= 10; x--)
	{
		if (((ssid[25] >> y) & 0x1) == 1)
			ssid[x] = ssid[x] | 0x80;
		y++;
	}
    
    y = 0;
	for (x = 9; x >= 3; x--)
	{
		if (((ssid[24] >> y) & 0x1) == 1)
			ssid[x] = ssid[x] | 0x80;
		y++;
	}
	
	//Now we can easily populate the struct

	decoded_data.dev_id			= ssid[ 3] << 16 | ssid[ 4] <<  8 | ssid[ 5];
	decoded_data.lat			= ssid[ 6] << 24 | ssid[ 7] << 16 | ssid[ 8] <<  8 | ssid[ 9];
	decoded_data.lng			= ssid[10] << 24 | ssid[11] << 16 | ssid[12] <<  8 | ssid[13];
 	decoded_data.app_id			= ssid[14] << 16 | ssid[15] <<  8 | ssid[16];  
	decoded_data.altitude		= ssid[17] << 18 | ssid[18] << 10 | ssid[19] <<  2 | ((ssid[20] >> 6) & 0x03);
    if (((ssid[20] & 0x20) >> 5) & 1) decoded_data.altitude = (decoded_data.altitude * -1);
	decoded_data.off_map		= ((ssid[20] & 0x10) >> 4) & 1;
	decoded_data.three_d_map	= ((ssid[20] & 0x08) >> 3) & 1;
	decoded_data.tx_pwr			= ((ssid[20] & 0x07) << 8) | ssid[21];
    decoded_data.tx_pwr         = decoded_data.tx_pwr - 1000;
    decoded_data.path_loss   	= ( ssid[22] & 0xE0) >> 5;  
    decoded_data.res			= (ssid[22]  & 0x1F  << 8) | ssid[23];
	
	return decoded_data;
	
};

main (int argc, char *argv[])
{
	unsigned char *encoded_string = "";
 	struct sps_data encode_data; 
    struct sps_data decoded_data; 	
    
	encode_data.dev_id		= 99;
	encode_data.lat			= 891234567;
	encode_data.lng			= 997654321;
	encode_data.altitude	= 12345678;
	encode_data.tx_pwr		= -900;
	encode_data.off_map		= 1;
	encode_data.three_d_map = 1;
	encode_data.path_loss   = 2;
	encode_data.res			= 0x2f;
	encode_data.app_id		= 99;
    
	
	printf("Raw Data\n------------\n");
	printf("Device ID    : %u\n", encode_data.dev_id  );
	printf("Latitude     : %d\n", encode_data.lat     );
	printf("Longitude    : %d\n", encode_data.lng     );
	printf("Altitude     : %d\n", encode_data.altitude);
	printf("Tx Power     : %d\n", encode_data.tx_pwr  );
	printf("Alt Mapping  : %d\n", encode_data.off_map );
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
	printf("\"\n\n\"");
	for(i = 0; i < 31; i++)
        printf("0x%02x,", encoded_string[i]);
		//printf("%02d,", encoded_string[i]);
	printf("\"\n\n");
    for(i = 0; i < 31; i++){
        printf("%%", encoded_string[i]);
        printf("%02x", encoded_string[i]);
        }

	printf("\n\n");
	
	decoded_data   = decode_ssid(encoded_string);
	
	printf("Decoded SSID\n------------\n");
	printf("Device ID    : %u\n", decoded_data.dev_id  );
	printf("Latitude     : %d\n", decoded_data.lat     );
	printf("Longitude    : %d\n", decoded_data.lng     );
	printf("Altitude     : %d\n", decoded_data.altitude);
	printf("Tx Power     : %d\n", decoded_data.tx_pwr  );
	printf("Alt Mapping  : %d\n", decoded_data.off_map );
	printf("3D Mapping   : %d\n", encode_data.three_d_map );
	printf("Path Loss    : %d\n", encode_data.path_loss );
	printf("Reserved     : %x\n", decoded_data.res     );
	printf("App ID       : %u\n", decoded_data.app_id  );
	
	return 0;
	
};