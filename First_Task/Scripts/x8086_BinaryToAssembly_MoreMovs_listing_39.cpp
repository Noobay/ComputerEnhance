
#include <iostream>
#include <fstream>

// 8 bits - [100010,(d)x,(w)x]
const uint8_t COMMAND_MOVE_REGISTER_MEM_TO_FROM_REGISTER = 0b100010;
// 8 bits - [1011,(w)x,(reg)xxx]
const uint8_t COMMAND_MOVE_IMMEDIATE_MEM_TO_FROM_REGISTER = 0b1011;

const int BUFF_SIZE = 64;
void getRegName(char source[2], uint8_t mode, uint8_t word);

const int MAX_16_BITNUMBER_CHAR_COUNT = 5; // 2^16 => 65536

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

    uint8_t fileData[BUFF_SIZE];

    if (filePointer != 0)
    {
        int byteCount = fread_s(fileData, BUFF_SIZE, 1, BUFF_SIZE, filePointer);

        printf("bits 16\n");

        char registerName[3];
        char registerOrMemoryAddr[BUFF_SIZE];

        registerName[2] = '\0';
        registerOrMemoryAddr[2] = '\0';

        // Assuming Little Endian
        for (int i = 0; i < byteCount; ++i)
        {
            uint8_t commandByte1 = fileData[i];
            if ((commandByte1 >> 4) == COMMAND_MOVE_IMMEDIATE_MEM_TO_FROM_REGISTER)
            {
                uint8_t word = (commandByte1 >> 3) & 0b1;
                uint8_t reg = commandByte1 & 0b111;
                uint16_t data = fileData[++i];

                getRegName(registerName, reg, word);
                if (word == 1)
                    data |= (fileData[++i] << 8);

                snprintf(registerOrMemoryAddr, sizeof(registerOrMemoryAddr), "%u", data);
                printf("mov %s, %s\n", registerName, registerOrMemoryAddr);
            }
            else if ((commandByte1 >> 2) == COMMAND_MOVE_REGISTER_MEM_TO_FROM_REGISTER)
            { 
                uint8_t direction = (commandByte1 >> 1) & 0b1;
                uint8_t word = commandByte1 & 0b1;

                uint8_t commandByte2 = fileData[++i];
                uint8_t mode = (commandByte2 >> 6) & 0b11;
                uint8_t reg = (commandByte2 >> 3) & 0b111;
                uint8_t reg_mem = commandByte2 & 0b111;

                getRegName(registerName, reg, word);
                switch (mode)
                {
                    case 0b11:
                        getRegName(registerOrMemoryAddr, reg_mem, word);
                        break;
                    case 0b00:
                        switch (reg_mem)
                        {
                        case 0b000:
                            memcpy(registerOrMemoryAddr, "[bx + si]\0", sizeof(char) * 10);
                            break;
                        case 0b001:
                            memcpy(registerOrMemoryAddr, "[bx + di]\0", sizeof(char) * 10);
                            break;
                        case 0b010:
                            memcpy(registerOrMemoryAddr, "[bp + si]\0", sizeof(char) * 10);
                            break;
                        case 0b011:
                            memcpy(registerOrMemoryAddr, "[bp + di]\0", sizeof(char) * 10);
                            break;
                        case 0b100:
                            memcpy(registerOrMemoryAddr, "si\0", sizeof(char) * 3);
                            break;
                        case 0b101:
                            memcpy(registerOrMemoryAddr, "di\0", sizeof(char) * 3);
                            break;
                        case 0b111:
                            memcpy(registerOrMemoryAddr, "bx\0", sizeof(char) * 3);
                            break;
                        case 0b110:
                            uint8_t lowDisp = fileData[++i];
                            uint8_t highDisp = fileData[++i];
                            uint16_t value = (highDisp << 8) | lowDisp;
                            snprintf(registerOrMemoryAddr, BUFF_SIZE, "[%u]", value);
                            
                            // Special Case DIRECT ADDRESS 
                            break;
                        }
                        break;
                    case 0b01:
                    case 0b10:
                    {
                        uint16_t data = fileData[++i];
                        getRegName(registerName, reg, word);
                        if (mode == 0b10)
                            data |= (fileData[++i] << 8);

                        switch (reg_mem)
                        {
                        case 0b000:
                            snprintf(registerOrMemoryAddr, BUFF_SIZE, "[bx + si + %u]", data);
                            break;
                        case 0b001:
                            snprintf(registerOrMemoryAddr, BUFF_SIZE, "[bx + di + %u]", data);
                            break;
                        case 0b010:
                            snprintf(registerOrMemoryAddr, BUFF_SIZE, "[bp + si + %u]", data);
                            break;
                        case 0b011:
                            snprintf(registerOrMemoryAddr, BUFF_SIZE, "[bp + di + %u]", data);
                            break;
                        case 0b100:
                            snprintf(registerOrMemoryAddr, BUFF_SIZE, "[si + %u]", data);
                            break;
                        case 0b101:
                            snprintf(registerOrMemoryAddr, BUFF_SIZE, "[di + %u]", data);
                            break;
                        case 0b110:
                            snprintf(registerOrMemoryAddr, BUFF_SIZE, "[bp + %u]", data);
                            break;
                        case 0b111:
                            snprintf(registerOrMemoryAddr, BUFF_SIZE, "[bx + %u]", data);
                            break;
                        }

                        break;
                    }
                }

                if (direction == 1)
                    printf("mov %s, %s", registerName, registerOrMemoryAddr);
                else 
                    printf("mov %s, %s", registerOrMemoryAddr, registerName);

                printf("\n");
            }                

            
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