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
    var latCheck = document.forms["frmCoder"]["lat"].value;
    var latInt = document.forms["frmCoder"]["lat"].value.split(".")[0];
    if (document.forms["frmCoder"]["lat"].value.includes("."))
    {
        var latDec = document.forms["frmCoder"]["lat"].value.split(".")[1];
    } else {
        var latDec = 0000000;
    }
    latDec = pad_right(latDec, "0", 7);
    lat = parseInt(latInt + latDec);
    var lngCheck = document.forms["frmCoder"]["long"].value;
    var lngInt = document.forms["frmCoder"]["long"].value.split(".")[0];
    if (document.forms["frmCoder"]["long"].value.includes("."))
    {
        var lngDec = document.forms["frmCoder"]["long"].value.split(".")[1];
    } else {
        var lngDec = "0000000";
    }

    lngDec = pad_right(lngDec, "0", 7);
    lng = parseInt(lngInt + lngDec);
    var altitude = document.forms["frmCoder"]["alt"].value;
    var tx_pwrCheck = document.forms["frmCoder"]["txpwr"].value;
    var tx_pwrInt = document.forms["frmCoder"]["txpwr"].value.split(".")[0];
    if (document.forms["frmCoder"]["txpwr"].value.includes("."))
    {
        var tx_pwrDec = document.forms["frmCoder"]["txpwr"].value.split(".")[1];
    } else {
        var tx_pwrDec = "0";
    }

    tx_pwr = parseInt(tx_pwrInt + tx_pwrDec);
    var off_map = document.forms["frmCoder"]["off"].checked;
    var three_d_map = document.forms["frmCoder"]["3d"].checked;
    var path_loss = document.forms["frmCoder"]["path"].value;
    var res = document.forms["frmCoder"]["res"].value;
    var invalid = document.forms["frmCoder"]["invalid"].value;
    
    
    /* if (lng == null || lng == "") {
        alert("Longitude must be filled out");
        return false;
    }
    if (lat == null || lat == "") {
        alert("Latitude must be filled out");
        return false;
    }*/
    if (latCheck > 90 || latCheck < -90){
        alert("Please enter valid latitude value (-90 to 90).");
        return false;
    }
    if (lngCheck > 180 || lngCheck < -180){
        alert("Please enter valid latitude value (-180 to 180).");
        return false;
    }
    if (tx_pwrCheck < -100 || tx_pwrCheck > 23){
        alert("Please enter valid tx value (-100 to 23dbm).");
        return false;
    }
    if (app_id > 16777215){
        alert("Please enter valid 24 bit application ID.");
        return false;
    }
    if (dev_id > 16777215){
        alert("Please enter valid 24 bit application ID.");
        return false;
    }
    if (altitude > 67108863 || altitude < -67108863){
        alert("Please enter valid altitude.");
        return false;
    }
    
    var re = new RegExp( /^[0-9A-Fa-f]+$/ );
    if (!(re.test(res))) {
        alert("Please enter valid reserved hex value.");
        return false;
    }
    
    if (parseInt(res, 16) > 4095) {
        alert("Please enter valid reserved hex value.");
        return false;
    }
    invalid = invalid.trim();
    var invalid_chars = invalid.split(" ");
    invalid_chars = uniq(invalid_chars);
    

    if (invalid.length > 0) {
        for (index = 0; index < invalid_chars.length; ++index) {

            if (!(re.test(invalid_chars[index]))) {
                alert("Please enter valid invalid chars.");
                return false;
            }
        }
        
        //Note 0x7f can never be encoded out if 0x00 is encoded out.
        if ((invalid.indexOf("7f") !=-1) & (invalid.indexOf("00") !=-1)) {
            alert("Cannot code both 0x7f and 0x00 out.");
            return false;
        }
        
        var test_invalid = [];
        
        //Test that two adjacent hex values aren't being coded out.
        for (index = 0; index < invalid_chars.length; ++index) {
            test_invalid.push(parseInt(invalid_chars[index], 16));
        }
        test_invalid = test_invalid.sort();
        var prev;
        for (index = 0; index < test_invalid.length; ++index) {
            
            if (prev == test_invalid[index]) {
                alert("Cannot have two invalid chars adjacent to each other.");
                return false;
            }
            prev = test_invalid[index] +1;
        }
    }
    
    
    
    //lat = lat * 10000000;
    //lng = lng * 10000000;

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
                       | boolToVal(altitude < 0)  << 5
                       | boolToVal(off_map)       << 4 
                       | boolToVal(three_d_map)   << 3 
                       |((tx_pwr + 1000)>> 8   & 0x07);
    encoded_string[21] = (tx_pwr + 1000)       & 0xFF;
    

    encoded_string[22] = ((parseInt(path_loss)  & 0x07)  << 5)
                       |((parseInt(res, 16) >> 8)   & 0x1F);
    encoded_string[23] = (parseInt(res, 16)     )   & 0xFF;
    

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

    var found = false;
    var coding_mask = 0;
    if (invalid.length > 0) {
        for (x = 3; x <= 26; x++)
        {
            found = false;
            for (index = 0; index < invalid_chars.length; ++index) {
                if (encoded_string[x] == parseInt(invalid_chars[index], 16)) {
                    found = true;
                }
            }
            if (found == true) {
                coding_mask = (coding_mask << 1) | 1;
                if (encoded_string[x] == 0x7f) { 
                    encoded_string[x] = 0;
                } else {
                    encoded_string[x] = encoded_string[x] + 1;
                }
            } else {
                coding_mask = (coding_mask << 1) | 0;
            }
            //printf("%x\n",coding_mask);
        }
    }
    //Shift to leave 4 bits to encode out control chars from coding mask .
    coding_mask = coding_mask << 4;
    var byte_temp = 0;
    var bit_mask;
    //Now check the coding mask for any control chars. Treat as 7 bit word
    if (invalid.length > 0) {
        for (x = 3; x >= 0; x--)
        {
            bit_mask = 0x7F << x*7;
            byte_temp = ((coding_mask >> x*7) & 0x7F);
            found = false;
            for (index = 0; index < invalid_chars.length; ++index) {
                if (byte_temp == parseInt(invalid_chars[index], 16)) {
                    found = true;
                }
            }     
            if (found == true) {
                if (byte_temp == 0x7f) {
                    byte_temp = 0;
                } else {
                    byte_temp = byte_temp + 1;
                }
                coding_mask = (coding_mask & ~bit_mask) | (byte_temp << x*7);
                coding_mask = coding_mask  | (1 << x);
            }
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

function uniq(a) {
    var seen = {};
    var out = [];
    var len = a.length;
    var j = 0;
    for(var i = 0; i < len; i++) {
         var item = a[i];
         if(seen[item] !== 1) {
               seen[item] = 1;
               out[j++] = item;
         }
    }
    return out;
};
function toHex(str) {
    var hex = '';
    for(var i=0;i<str.length;i++) {
    
        hex += '\\x'+("00" + str.charCodeAt(i).toString(16)).substr(-2);
    }
    return hex;
};
function boolToVal(bool) {
    if (bool == true) {
        return 1;
    } else {
        return 0;
    }
};
// right padding s with c to a total of n chars

function pad_right(s, c, n) {
  if (! s || ! c || s.length >= n) {
    return s;
  }
  var max = (n - s.length)/c.length;
  for (var i = 0; i < max; i++) {
    s += c;
  }
  return s;
};