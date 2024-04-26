
#include <iostream>
#include <fstream>

const uint8_t COMMAND_MOVE_REGISTER_MEM_TO_FROM_REGISTER = 0b100010;

void getRegName(char source[2], uint8_t mode, uint8_t word);

int main(int argumentsCount, char** argumentsValue)
{
    if (argumentsCount <= 1)
    {
        printf("No Arguments were provided, please provide a path to a binary file");
        return 1;
    }

    char* filePath = argumentsValue[1];

    FILE* filePointer;
    errno_t error = fopen_s(&filePointer, filePath, "r");
    if (error != 0)
        printf("Error opening file"); 
    
    const int BUFF_SIZE = 128;
    uint16_t fileData[BUFF_SIZE];

    if (filePointer != 0)
    {
        int lineCount = fread_s(fileData, BUFF_SIZE, 2, BUFF_SIZE / 2, filePointer);

        printf("bits 16\n");

        char source[3];
        char destination[3];

        source[2] = '\0';
        destination[2] = '\0';

        // Little Endian
        for (int i = 0; i < lineCount; ++i)
        {
            uint16_t log = fileData[i];
            
            uint8_t command = (log >> 2) & 0b111111;
            uint8_t direction = (log >> 1) & 0b1;
            uint8_t word = log & 0b1;

            uint8_t mode = (log >> (8 + 6)) & 0b11;
            uint8_t reg = (log >> (8 + 3)) & 0b111;
            uint8_t reg_mem =  (log >> 8) & 0b111;

            getRegName(source, reg, word);

            if (mode == 0b00000011)
                getRegName(destination, reg_mem, word);

            if (command == COMMAND_MOVE_REGISTER_MEM_TO_FROM_REGISTER)
                printf("mov %s, %s", destination, source);

            printf("\n");
        }
    }

    return 0;

}

inline void getRegName(char buff[2], uint8_t reg, uint8_t word)
{
    if (word == 0)
    {
        switch (reg)
        {
            case 0b000:
                buff[0] = 'a';
                buff[1] = 'l';
                break;
            case 0b001:
                buff[0] = 'c';
                buff[1] = 'l';
                break;
            case 0b010:
                buff[0] = 'd';
                buff[1] = 'l';
                break;
            case 0b011:
                buff[0] = 'b';
                buff[1] = 'l';
                break;
            case 0b100:
                buff[0] = 'a';
                buff[1] = 'h';
                break;
            case 0b101:
                buff[0] = 'c';
                buff[1] = 'h';
                break;
            case 0b110:
                buff[0] = 'd';
                buff[1] = 'h';
                break;
            case 0b111:
                buff[0] = 'b';
                buff[1] = 'h';
                break;
        }
    }
    else
    {
        switch (reg)
        {
            case 0b000:
                buff[0] = 'a';
                buff[1] = 'x';
                break;
            case 0b001:
                buff[0] = 'c';
                buff[1] = 'x';
                break;
            case 0b010:
                buff[0] = 'd';
                buff[1] = 'x';
                break;
            case 0b011:
                buff[0] = 'b';
                buff[1] = 'x';
                break;
            case 0b100:
                buff[0] = 's';
                buff[1] = 'p';
                break;
            case 0b101:
                buff[0] = 'b';
                buff[1] = 'p';
                break;
            case 0b110:
                buff[0] = 's';
                buff[1] = 'i';
                break;
            case 0b111:
                buff[0] = 'd';
                buff[1] = 'i';
                break;
        }

    }
}