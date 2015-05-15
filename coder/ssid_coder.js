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
 */


function encode_ssid() {

    //The tag is always valid SSID charset
    var tag = "SPS";


    var dev_id = document.forms["frmCoder"]["devid"].value;
    var app_id = document.forms["frmCoder"]["appid"].value;
    var lat = document.forms["frmCoder"]["lat"].value;
    var lng = document.forms["frmCoder"]["long"].value;
    var altitude = document.forms["frmCoder"]["alt"].value;
    var tx_pwr = document.forms["frmCoder"]["txpwr"].value;
    var off_map = document.forms["frmCoder"]["off"].checked;
    var three_d_map = document.forms["frmCoder"]["3d"].checked;
    var path_loss = document.forms["frmCoder"]["path"].value;
    var res = document.forms["frmCoder"]["res"].value;
    
    
    if (lng == null || lng == "") {
        alert("Longitude must be filled out");
        return false;
    }
    if (lat == null || lat == "") {
        alert("Latitude must be filled out");
        return false;
    }
    if (lat > 90 || lat < -90){
        alert("Please enter valid latitude value (-90 to 90)");
        return false;
    }
    if (lng > 180 || lng < -180){
        alert("Please enter valid latitude value (-180 to 180)");
        return false;
    }
    lat = lat * 10000000;
    lng = lng * 10000000;

	var encoded_string = new Uint8Array(31);
	
	//For the string data, we can just read it raw; nothing fancy here.
	//Encoding SSIDs doesn't happen often.
	
	//SPS TAG
	encoded_string[ 0] = tag.charCodeAt(0);
	encoded_string[ 1] = tag.charCodeAt(1);
	encoded_string[ 2] = tag.charCodeAt(2);
	
	//Device ID
	encoded_string[ 3] = (dev_id        >> 16) & 0xFF;
	encoded_string[ 4] = (dev_id        >>  8) & 0xFF;
	encoded_string[ 5] = (dev_id             ) & 0xFF;
	
	//Latitude
	encoded_string[ 6] = (lat           >> 24) & 0xFF;
	encoded_string[ 7] = (lat           >> 16) & 0xFF;
	encoded_string[ 8] = (lat           >>  8) & 0xFF;
	encoded_string[ 9] = (lat                ) & 0xFF;
	
	//Longitude
	encoded_string[10] = (lng           >> 24) & 0xFF;
	encoded_string[11] = (lng           >> 16) & 0xFF;
	encoded_string[12] = (lng           >>  8) & 0xFF;
	encoded_string[13] = (lng                ) & 0xFF;
    
    //Application ID
	encoded_string[14] = (app_id        >> 16) & 0xFF;
	encoded_string[15] = (app_id        >>  8) & 0xFF;
	encoded_string[16] = (app_id             ) & 0xFF;
	
	//Altitude
	encoded_string[17] = (Math.abs(altitude) >> 18) & 0xFF;
	encoded_string[18] = (Math.abs(altitude) >> 10) & 0xFF;
	encoded_string[19] = (Math.abs(altitude) >>  2) & 0xFF;
	
	encoded_string[20] =((Math.abs(altitude)        & 0x03) << 6)
                       | (altitude < 0) << 5
                       |  off_map       << 4 
	                   |  three_d_map   << 3 
                       |((tx_pwr + 1000)>> 8   & 0x07);
    encoded_string[21] = (tx_pwr + 1000)       & 0xFF;
	

	encoded_string[22] = (path_loss     << 5)  & 0xE0
	                   |((res >> 8) & 0x1F);
	encoded_string[23] = (res               )  & 0xFF;
	

	
    //Transform encoded string and create "ASCII" 7bit mask
    var x;
    var ascii_mask = 0; //21 bits (for 21 bytes) to encode
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

	var coding_mask = 0;
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
    var byte_temp = 0;
	var bit_mask;
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
   
	
	document.getElementById("output").value = String.fromCharCode.apply(null, encoded_string);
    
    document.getElementById("output_hex").value = toHex(String.fromCharCode.apply(null, encoded_string));
	
};
function toHex(str) {
	var hex = '';
	for(var i=0;i<str.length;i++) {
    
		hex += '\\x'+("00" + str.charCodeAt(i).toString(16)).substr(-2);
	}
	return hex;
};