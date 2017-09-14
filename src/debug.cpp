/**
    This file is for debug utilities that might be used if a problem is found in the code
**/

/** Print in hexadecimal notation an array of bytes.
    @param data pointer to char
    @param data_size size of the array pointed by data
**/
void hex_print(const char* data, size_t data_size)
    for (uint8_t i=0; i<data_size; ++i)
    {
        if (i>0)
            Serial.print(' ');
        Serial.print((int)data[i], HEX);
    }
}