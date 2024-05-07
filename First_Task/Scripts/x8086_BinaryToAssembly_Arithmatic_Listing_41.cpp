#include <iostream>

// 8 bits - [100010,(d)x,(w)x]
const uint8_t COMMAND_MOVE_REGISTER_MEM_TO_FROM_REGISTER = 0b100010;
// 8 bits - [1100011, (w)x]
const uint8_t COMMAND_MOVE_IMMEDIATE_TO_REGISTER_MEM = 0b1100011;
// 8 bits - [1010000, (w)x]
const uint8_t COMMAND_MOVE_MEMORY_TO_ACCUMULATOR = 0b1010000;
// 8 bits - [1010001, (w)x]
const uint8_t COMMAND_MOVE_ACCUMULATOR_TO_MEMORY = 0b1010001;
// 8 bits - [1011,(w)x,(reg)xxx]
const uint8_t COMMAND_MOVE_IMMEDIATE_MEM_TO_REGISTER = 0b1011;

const uint8_t COMMAND1_ARITHMETIC_PREFIX = 0b00;

// 8 bits - [00, (op)XXX, 10, (w)x]
const uint8_t COMMAND3_ARITHMETIC_IMMEDIATE_TO_ACCUMULATOR = 0b10;
// 8 bits - [00, (op)XXX,  0, (d)x, (w)x]
const uint8_t COMMAND3_ARITHMETIC_REGISTER_MEM_TO_REGISTER_MEM = 0b0;

// 8 bits - [100000, (s)x, (w)x]
const uint8_t COMMAND1_ARITHMETIC_IMMEDIATE_TO_MEMORY = 0b100000;

// 3 bits - [(mod)xx (op)XXX (r/m)xxx]
const uint8_t COMMAND2_ARITHMETIC_ADD_MASK = 0b000;
const uint8_t COMMAND2_ARITHMETIC_SUB_MASK = 0b101;
const uint8_t COMMAND2_ARITHMETIC_CMP_MASK = 0b111;

const char OP_ADD_NAME[] = "add";
const char OP_SUB_NAME[] = "sub";
const char OP_CMP_NAME[] = "cmp";

const char SIZE_DESCRIPTOR_BYTE[] = "byte";
const char SIZE_DESCRIPTOR_WORD[] = "word";

const int FILE_BUFF_SIZE = 256;
const int TEXT_BUFF_SIZE = 64;

const int MAX_16BIT_DIGIT_COUNT = 5; // (worst case - uint16) 2^16 => 65536
const int MAX_DISPLACEMENT_TEXT_SIZE = MAX_16BIT_DIGIT_COUNT + 2; // [16_BIT_DIGIT]

const uint16_t EXTEND_SIGN_MASK = (0b11111111 << 8);

void getRegName(char source[3], uint8_t mode, uint8_t word);
void getDisplacementText(int16_t value, char text[MAX_DISPLACEMENT_TEXT_SIZE]);
int getRegisterOrMemoryText(char outTextBuff[TEXT_BUFF_SIZE], char tmpDisplacementTextBuff[MAX_DISPLACEMENT_TEXT_SIZE], uint8_t* dataBuff, int dataBuffSize, int activeDataBuffIdx, uint8_t word, uint8_t mode, uint8_t reg_mem);

int main(int argumentsCount, char** argumentsValue)
{
    if (argumentsCount <= 1)
    {
        printf("No Arguments were provided, please provide a path to a binary file");
        return 1;
    }

    char* filePath = argumentsValue[1];

    FILE* pFile;
    errno_t error = fopen_s(&pFile, filePath, "r");
    if (error != 0)
        printf("Error opening file");

    uint8_t fileData[FILE_BUFF_SIZE];

    if (pFile != 0)
    {
        int fileBytesCount = fread_s(fileData, FILE_BUFF_SIZE, 1, FILE_BUFF_SIZE, pFile);
        printf("bits 16\n");

        char registerName[3];
        char registerOrMemoryAddrText[TEXT_BUFF_SIZE];
        char displacementText[MAX_DISPLACEMENT_TEXT_SIZE];

        registerName[2] = '\0';
        int byteOffset = 0;
        while (byteOffset < fileBytesCount)
        {
            
            memset(registerOrMemoryAddrText, '\0', TEXT_BUFF_SIZE);
            memset(displacementText, '\0', MAX_DISPLACEMENT_TEXT_SIZE);

            uint8_t commandByte1 = fileData[byteOffset++];
	        if ((commandByte1 >> 1) == COMMAND_MOVE_MEMORY_TO_ACCUMULATOR)
            {
	        	uint8_t word = commandByte1 & 0b1;
                uint16_t data = fileData[byteOffset++];
		        if (word == 1)
                    data |= (fileData[byteOffset++] << 8);

                printf("mov ax, [%u]\n", data);

            }
            else if ((commandByte1 >> 1) == COMMAND_MOVE_ACCUMULATOR_TO_MEMORY)
            {
                uint8_t word = commandByte1 & 0b1;
                uint16_t data = fileData[byteOffset++];
		        if (word == 1)
                    data |= (fileData[byteOffset++] << 8);

                printf("mov [%u], ax\n", data);
            }           
            else if ((commandByte1 >> 1) == COMMAND_MOVE_IMMEDIATE_TO_REGISTER_MEM)
            {
                uint8_t word = commandByte1 & 0b1;

                uint8_t commandByte2 = fileData[byteOffset++];

                uint8_t mode = (commandByte2 >> 6) & 0b11;
                uint8_t reg_mem = commandByte2 & 0b111;

                byteOffset = getRegisterOrMemoryText(registerOrMemoryAddrText, displacementText, fileData, fileBytesCount, byteOffset, 1, mode, reg_mem);

                uint16_t data = fileData[byteOffset++];

                const char* sizeDescriptor = SIZE_DESCRIPTOR_BYTE;
                if (word == 1)
                {
                    data |= (fileData[byteOffset++] << 8);
                    sizeDescriptor = SIZE_DESCRIPTOR_WORD;
                }

                printf("mov %s, %s %u\n", registerOrMemoryAddrText, sizeDescriptor, data);
            }
            else if ((commandByte1 >> 2) == COMMAND1_ARITHMETIC_IMMEDIATE_TO_MEMORY)
            {
                uint8_t signExtend = commandByte1 >> 1 & 0b1;
                uint8_t word = commandByte1 & 0b1;

                uint8_t commandByte2 = fileData[byteOffset++];

                uint8_t mode = (commandByte2 >> 6) & 0b11;
                uint8_t arithmeticOpCode = commandByte2 >> 3 & 0b111;
                uint8_t reg_mem = commandByte2 & 0b111;

                byteOffset = getRegisterOrMemoryText(registerOrMemoryAddrText, displacementText, fileData, fileBytesCount, byteOffset, 1, mode, reg_mem);
                const char* opName = OP_ADD_NAME;
                switch (arithmeticOpCode)
                {
                    case COMMAND2_ARITHMETIC_ADD_MASK:
                        opName = OP_ADD_NAME;
                        break;
                    case COMMAND2_ARITHMETIC_SUB_MASK:
                        opName = OP_SUB_NAME;
                        break;
                    case COMMAND2_ARITHMETIC_CMP_MASK:
                        opName = OP_CMP_NAME;
                        break;
                    default:
                        char errorText[128];
                        snprintf(errorText, 128, "Unexpected operation code %u", arithmeticOpCode);
                        throw std::exception(errorText);
                        return 1;
                }

                int16_t data = fileData[byteOffset++];

                const char* sizeDescriptor = SIZE_DESCRIPTOR_BYTE;
                if (signExtend == 1 && word == 1)
                    sizeDescriptor = SIZE_DESCRIPTOR_WORD;
                else if (signExtend == 0 && word == 1)
                {
                    data |= (fileData[byteOffset++] << 8);
                    sizeDescriptor = SIZE_DESCRIPTOR_WORD;
                }
                else if (signExtend == 1)
                    data |= EXTEND_SIGN_MASK;
 
                printf("%s %s %s, %i\n", opName, sizeDescriptor, registerOrMemoryAddrText, data);
            }
            else if ((commandByte1 >> 2) == COMMAND_MOVE_REGISTER_MEM_TO_FROM_REGISTER)
            {
                uint8_t direction = (commandByte1 >> 1) & 0b1;
                uint8_t word = commandByte1 & 0b1;

                uint8_t commandByte2 = fileData[byteOffset++];
                uint8_t mode = (commandByte2 >> 6) & 0b11;
                uint8_t reg = (commandByte2 >> 3) & 0b111;
                uint8_t reg_mem = commandByte2 & 0b111;

                getRegName(registerName, reg, word);
                byteOffset = getRegisterOrMemoryText(registerOrMemoryAddrText, displacementText, fileData, fileBytesCount, byteOffset, word, mode, reg_mem);

                if (direction == 1)
                    printf("mov %s, %s\n", registerName, registerOrMemoryAddrText);
                else
                    printf("mov %s, %s\n", registerOrMemoryAddrText, registerName);
            }
            else if ((commandByte1 >> 4) == COMMAND_MOVE_IMMEDIATE_MEM_TO_REGISTER)
            {
                uint8_t word = (commandByte1 >> 3) & 0b1;
                uint8_t reg = commandByte1 & 0b111;
                uint16_t data = fileData[byteOffset++];

                getRegName(registerName, reg, word);
                if (word == 1)
                    data |= (fileData[byteOffset++] << 8);

                printf("mov %s, %u\n", registerName, data);
            }
            else if ( (commandByte1 >> 6) == COMMAND1_ARITHMETIC_PREFIX && 
                      ((commandByte1 >> 1) & 0b11) == COMMAND3_ARITHMETIC_IMMEDIATE_TO_ACCUMULATOR )
            {
                uint8_t word = commandByte1 & 0b1;
                uint8_t arithmeticOpCode = commandByte1 >> 3 & 0b111;

                const char* opName = OP_ADD_NAME;
                switch (arithmeticOpCode)
                {
                case COMMAND2_ARITHMETIC_ADD_MASK:
                    opName = OP_ADD_NAME;
                    break;
                case COMMAND2_ARITHMETIC_SUB_MASK:
                    opName = OP_SUB_NAME;
                    break;
                case COMMAND2_ARITHMETIC_CMP_MASK:
                    opName = OP_CMP_NAME;
                    break;
                default:
                    char errorText[128];
                    snprintf(errorText, 128, "Unexpected operation code %u", arithmeticOpCode);
                    throw std::exception(errorText);
                    return 1;
                }

                uint16_t data = fileData[byteOffset++];
		        if (word == 1)
                {
                    data |= (fileData[byteOffset++] << 8);
                    printf("%s ax, %u\n", opName, data);
                }
                else
                    printf("%s al, %u\n", opName, data);
            }
            else if ( (commandByte1 >> 6) == COMMAND1_ARITHMETIC_PREFIX &&
                      ((commandByte1 >> 2) & 0b1) == COMMAND3_ARITHMETIC_REGISTER_MEM_TO_REGISTER_MEM)
            {
                uint8_t arithmeticOpCode = commandByte1 >> 3 & 0b111;
                uint8_t direction = (commandByte1 >> 1) & 0b1;
                uint8_t word = commandByte1 & 0b1;

                uint8_t commandByte2 = fileData[byteOffset++];
                uint8_t mode = (commandByte2 >> 6) & 0b11;
                uint8_t reg = (commandByte2 >> 3) & 0b111;
                uint8_t reg_mem = commandByte2 & 0b111;

                getRegName(registerName, reg, word);
                byteOffset = getRegisterOrMemoryText(registerOrMemoryAddrText, displacementText, fileData, fileBytesCount, byteOffset, 1, mode, reg_mem);

                const char* opName = OP_ADD_NAME;
                switch (arithmeticOpCode)
                {
                case COMMAND2_ARITHMETIC_ADD_MASK:
                    opName = OP_ADD_NAME;
                    break;
                case COMMAND2_ARITHMETIC_SUB_MASK:
                    opName = OP_SUB_NAME;
                    break;
                case COMMAND2_ARITHMETIC_CMP_MASK:
                    opName = OP_CMP_NAME;
                    break;
                default:
                    char errorText[128];
                    snprintf(errorText, 128, "Unexpected operation code %u", arithmeticOpCode);
                    throw std::exception(errorText);
                    return 1;
                }

                if (direction == 1)
                    printf("%s %s, %s", opName, registerName, registerOrMemoryAddrText);
                else
                    printf("%s %s, %s", opName, registerOrMemoryAddrText, registerName);

                printf("\n");
            } 
        }
    }

    return 0;

}

inline int getRegisterOrMemoryText(char outTextBuff[TEXT_BUFF_SIZE], char tmpDisplacementTextBuff[MAX_DISPLACEMENT_TEXT_SIZE], uint8_t* dataBuff, int dataBuffSize, int activeDataBuffIdx, uint8_t word, uint8_t mode, uint8_t reg_mem)
{
    switch (mode)
    {
    case 0b11:
        getRegName(outTextBuff, reg_mem, word);
        break;
    case 0b00:
        switch (reg_mem)
        {
        case 0b000:
            memcpy(outTextBuff, "[bx + si]", sizeof(char) * 10);
            break;
        case 0b001:
            memcpy(outTextBuff, "[bx + di]", sizeof(char) * 10);
            break;
        case 0b010:
            memcpy(outTextBuff, "[bp + si]", sizeof(char) * 10);
            break;
        case 0b011:
            memcpy(outTextBuff, "[bp + di]", sizeof(char) * 10);
            break;
        case 0b100:
            memcpy(outTextBuff, "si", sizeof(char) * 3);
            break;
        case 0b101:
            memcpy(outTextBuff, "di", sizeof(char) * 3);
            break;
        case 0b111:
            memcpy(outTextBuff, "bx", sizeof(char) * 3);
            break;
        case 0b110:
            int nextDataBuffIdx = activeDataBuffIdx + 2;

            if (nextDataBuffIdx >= dataBuffSize)
            {
                char errorText[128];
                snprintf(errorText, 128, "dataBuff access out of bounds for [Active Idx ~ %i Next Idx ~ %i]", activeDataBuffIdx, nextDataBuffIdx);
                throw std::out_of_range(errorText);
            }
            
            uint8_t lowDisp = dataBuff[activeDataBuffIdx++];
            uint8_t highDisp = dataBuff[activeDataBuffIdx++];
            uint16_t value = (highDisp << 8) | lowDisp;
            snprintf(outTextBuff, TEXT_BUFF_SIZE, "[%u]", value);

            // Special Case DIRECT ADDRESS 
            break;
        }
        break;
    case 0b01:
    case 0b10:
    {
        int nextDataBuffIdx = activeDataBuffIdx + 1;
        if (mode == 0b10)
            ++nextDataBuffIdx;

        if (nextDataBuffIdx >= dataBuffSize)
        {
            char errorText[128];
            snprintf(errorText, 128, "dataBuff access out of bounds for [Active Idx ~ %i Next Idx ~ %i]", activeDataBuffIdx, nextDataBuffIdx);
            throw std::out_of_range(errorText);
        }

        int16_t data = dataBuff[activeDataBuffIdx++];
        if (mode == 0b10)
            data |= (dataBuff[activeDataBuffIdx++] << 8);
        else
        {
            uint8_t sign = data >> 7;
            if (sign == 1)
                data |= EXTEND_SIGN_MASK;
        }

        getDisplacementText(data, tmpDisplacementTextBuff);

        switch (reg_mem)
        {
            case 0b000:
                snprintf(outTextBuff, TEXT_BUFF_SIZE, "[bx + si %s]", tmpDisplacementTextBuff);
                break;
            case 0b001:
                snprintf(outTextBuff, TEXT_BUFF_SIZE, "[bx + di %s]", tmpDisplacementTextBuff);
                break;
            case 0b010:
                snprintf(outTextBuff, TEXT_BUFF_SIZE, "[bp + si %s]", tmpDisplacementTextBuff);
                break;
            case 0b011:
                snprintf(outTextBuff, TEXT_BUFF_SIZE, "[bp + di %s]", tmpDisplacementTextBuff);
                break;
            case 0b100:
                snprintf(outTextBuff, TEXT_BUFF_SIZE, "[si %s]", tmpDisplacementTextBuff);
                break;
            case 0b101:
                snprintf(outTextBuff, TEXT_BUFF_SIZE, "[di %s]", tmpDisplacementTextBuff);
                break;
            case 0b110:
                snprintf(outTextBuff, TEXT_BUFF_SIZE, "[bp %s]", tmpDisplacementTextBuff);
                break;
            case 0b111:
                snprintf(outTextBuff, TEXT_BUFF_SIZE, "[bx %s]", tmpDisplacementTextBuff);
                break;
        }

        break;
    }
    }

    return activeDataBuffIdx;
}

inline void getDisplacementText(int16_t value, char text[MAX_DISPLACEMENT_TEXT_SIZE])
{
    if (value >= 0)
        snprintf(text, MAX_DISPLACEMENT_TEXT_SIZE, "+ %i", value);
    else
    {
        int16_t positiveValue = ~value + 1;
        snprintf(text, MAX_DISPLACEMENT_TEXT_SIZE, "- %i", positiveValue);
    }
}

inline void getRegName(char buff[3], uint8_t reg, uint8_t word)
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
