#include "LexicalAnalysis.h"

const char* LexicalAnalysis::m_keyword[38] = 
{ 
	"void", "char", "int", "flaot", "double", 
	"short", "long", "signed", "unsigned",
	"struct", "union", "enum", "typedef", "sizeof",
	"auto", "static", "register", "extern", "const", "volatile",
	"return", "continue", "break","goto",
	"if", "else", "switch", "case", "default",
	"for", "do", "while",

	"id", "10num", "16num", "literal", "punct"
};
const char* LexicalAnalysis::m_punctuation("+-*/()[]{}!@#$%^&<>=|:;?~.,\'`\"");

LexicalAnalysis::LexicalAnalysis(const char* _fileName)
:
m_file(_fileName),
m_fout("output.txt"),
m_fileLen(0),
m_currPos(0),
m_lineNum(1),
m_wordNum(0),
m_bufferMoved(NULL),
m_isBackward(false),
m_keywordLen(32),
m_punctuationLen(strlen(m_punctuation))
{
	memset(m_bufferA, 0, BUFFER_LEN);
	memset(m_bufferB, 0, BUFFER_LEN);
	if (m_file.is_open())
	{
		m_file.seekg(0, ios::end);
		m_fileLen = (int)m_file.tellg();
		m_file.seekg(0, 0);
		m_file.read(m_bufferA, BUFFER_LEN);
		m_bufferMoved = m_bufferA;
	}
}

LexicalAnalysis::LexicalAnalysis(const LexicalAnalysis& _lexicalAnalysis)
{
	if (!m_file.is_open())
	{
		m_file.close();
	}
	if (!m_fout.is_open())
	{
		m_fout.close();
	}
}

LexicalAnalysis::~LexicalAnalysis()
{
}

bool LexicalAnalysis::Analyze()
{
	if (!m_file.is_open())
	{
		cout << "File loading error" << endl;
		return false;
	}

	int temp = -1;
	while (1)
	{
		//move ptr to the beginning of the next word
		if (!DeleteNonsense())
			return true;

		//analyze the word
		pair<Token, string> data;
		//the end
		if (IsEnd(*m_bufferMoved))
		{
			return true;
		}
		//begin with the number
		else if (IsDecimalNumber(*m_bufferMoved))
		{
			data.first = GetNumber(data.second);
			if (data.first == ERROR)
			{
				cout << "ERROR:" << data.second;
				continue;
			}
			++m_wordNum;
		}
		//begin with the alpha
		else if (IsAlphaOrUnderline(*m_bufferMoved))
		{
			data.first = GetKeywordOrIdentifier(data.second);
			if (data.first == ERROR)
			{
				cout << "ERROR:" << data.second;
				continue;
			}
			++m_wordNum;
		}
		//begin with the punctuation
		else
		{
			data.first = GetPunctuation(data.second);
			if (data.first == ERROR)
			{
				cout << "ERROR:" << data.second;
				continue;
			}
		}
		//save the word
		m_data.push_back(data);

		if (temp != m_lineNum)
		{
			temp = m_lineNum;
			std::cout << "第" << temp << "行" << endl;
			m_fout << "第" << temp << "行" << endl;
		}

		std::cout << "\t" << m_keyword[data.first] << "\t" << data.second << endl;
		m_fout << "\t" << m_keyword[data.first] << "\t" << data.second << endl;
	}

	return true;
}

void LexicalAnalysis::Print()
{
	std::cout << "文件长度：" << m_fileLen << "\t文件行数：" << m_lineNum << "\t单词数量" << m_wordNum << endl;
	m_fout << "文件长度：" << m_fileLen << "\t文件行数：" << m_lineNum << "\t单词数量" << m_wordNum << endl;
}

bool LexicalAnalysis::Forward()
{
	if ((m_currPos + 1) % BUFFER_LEN)
	{
		++m_bufferMoved;
		++m_currPos;
	}
	else
	{
		if (((m_currPos + 1) / BUFFER_LEN) % 2)
		{
			if (m_isBackward)
			{
				m_bufferMoved = m_bufferA;
				++m_currPos;
				m_isBackward = false;
			}
			else
			{
				memset(m_bufferA, 0, BUFFER_LEN);
				m_file.read(m_bufferA, BUFFER_LEN);
				m_bufferMoved = m_bufferA;
				++m_currPos;
			}
		}
		else
		{
			if (m_isBackward)
			{
				m_bufferMoved = m_bufferB;
				++m_currPos;
				m_isBackward = false;
			}
			else
			{
				memset(m_bufferB, 0, BUFFER_LEN);
				m_file.read(m_bufferB, BUFFER_LEN);
				m_bufferMoved = m_bufferB;
				++m_currPos;
			}
		}
	}

	return true;
}

bool LexicalAnalysis::Backward()
{
	if (m_currPos % BUFFER_LEN)
	{
		--m_bufferMoved;
		--m_currPos;
	}
	else
	{
		if ((m_currPos / BUFFER_LEN) % 2)
		{
			m_bufferMoved = m_bufferA + BUFFER_LEN - 1;
			--m_currPos;
			m_isBackward = true;
		}
		else
		{
			m_bufferMoved = m_bufferB + BUFFER_LEN - 1;
			--m_currPos;
			m_isBackward = true;
		}
	}

	return true;
}

bool LexicalAnalysis::DeleteNonsense()
{
	//delete ' ','\t','\n','\r'
	while (IsNonsense(*m_bufferMoved))
	{
		if (*m_bufferMoved == '\n')
			++m_lineNum;
		if (!Forward())
			return false;
	}

	//deal with the comment
	if (*m_bufferMoved == '/')
	{
		//get the comment "//" (if there is a '\' in the end of the line except the white space, I need to ignore the '\n'.)
		if (*(m_bufferMoved + 1) == '/')
		{
			if (!Forward())
				return false;
			if (!Forward())
				return false;
			while (*m_bufferMoved != '\n')
			{
				if (!Forward())
					return false;
			}
			++m_lineNum;
			if (!Forward())
				return false;
			DeleteNonsense();
		}
		//get the comment beginning "/*"
		else if (*(m_bufferMoved + 1) == '*')
		{
			if (!Forward())
				return false;
			if (!Forward())
				return false;
			bool finished = false;
			while (!finished)
			{
				if (*m_bufferMoved == '*')
				{
					if (!Forward())
						return false;
					//get the comment ending "*/"
					if (*m_bufferMoved == '/')
					{
						if (!Forward())
							return false;
						finished = true;
					}
				}
				else
				{
					if (*m_bufferMoved == '\n')
						++m_lineNum;
					if (!Forward())
						return false;
				}
			}
			DeleteNonsense();
		}
	}
	return true;
}

Token LexicalAnalysis::GetNumber(string& _string)
{
	if (*m_bufferMoved == '0')
	{
		//get the hexadecimal number beginning "0x" or "0X"
		if (*(m_bufferMoved + 1) == 'x' || *(m_bufferMoved + 1) == 'X')
		{
			_string += *m_bufferMoved;
			if (!Forward())
				return ERROR;
			_string += *m_bufferMoved;
			if (!Forward())
				return ERROR;
			//0x(0~f)+
			if (IsHexadecimalNumber(*m_bufferMoved))
			{
				while (IsHexadecimalNumber(*m_bufferMoved))
				{
					_string += *m_bufferMoved;
					if (!Forward())
						return ERROR;
				}
			}
			else
			{
				_string = "there is only a 0x, whitch is illegal";
				return ERROR;
			}
			if (IsAlphaOrUnderline(*m_bufferMoved))
			{
				_string += " with alpha or underline is illegal";
				return ERROR;
			}
			return HEXADECIMALNUM;
		}
	}

	//(0~9)+
	while (IsDecimalNumber(*m_bufferMoved))
	{
		_string += *m_bufferMoved;
		if (!Forward())
			return ERROR;
	}
	//(0~9)+(.)?
	if (IsDot(*m_bufferMoved))
	{
		_string += *m_bufferMoved;
		if (!Forward())
			return ERROR;
		//(0~9)+(.(0~9)*)?
		while (IsDecimalNumber(*m_bufferMoved))
		{
			_string += *m_bufferMoved;
			if (!Forward())
				return ERROR;
		}
	}
	if (IsAlphaOrUnderline(*m_bufferMoved))
	{
		_string += " with alpha or underline is illegal";
		return ERROR;
	}
	return DECIMALNUM;
}

Token LexicalAnalysis::GetKeywordOrIdentifier(string& _string)
{
	while (IsAlphaOrUnderline(*m_bufferMoved))
	{
		_string += *m_bufferMoved;
		if (!Forward())
			return ERROR;
	}
	while (IsAlphaOrNumberOrUnderline(*m_bufferMoved))
	{
		_string += *m_bufferMoved;
		if (!Forward())
			return ERROR;
	}
	for (int i = 0; i < m_keywordLen; ++i)
	{
		if (_string == m_keyword[i])
		{
			//the word is a keyword
			return (Token)i;
		}
	}

	//the word is an id
	return ID;
}

//+-*/()[]{}!@#$%^&<>=|:;?~.,\'`\"
Token LexicalAnalysis::GetPunctuation(string& _string)
{
	int i = 0;
	for (; i < m_punctuationLen; ++i)
	{
		if (*m_bufferMoved == m_punctuation[i])
		{
			_string += *m_bufferMoved;
			if (!Forward())
				return ERROR;
			break;
		}
	}
	//if it is not a punctuation
	if (i >= m_punctuationLen)
	{
		return ERROR;
	}

	switch (_string[_string.size() - 1])
	{
	case '+':
		//"++" "+="
		if (*m_bufferMoved == '+' || *m_bufferMoved == '=')
		{
			_string += *m_bufferMoved;
			if (!Forward())
				return ERROR;
		}
		return PUNCTUATION;

	case '-':
		//"--" "-=" "->"
		if (*m_bufferMoved == '-' || *m_bufferMoved == '=' || *m_bufferMoved == '>')
		{
			_string += *m_bufferMoved;
			if (!Forward())
				return ERROR;
		}
		return PUNCTUATION;

	case '*':
		//"*="
		if (*m_bufferMoved == '=')
		{
			_string += *m_bufferMoved;
			if (!Forward())
				return ERROR;
		}
		return PUNCTUATION;

	case '/':
		//"/="
		if (*m_bufferMoved == '=')
		{
			_string += *m_bufferMoved;
			if (!Forward())
				return ERROR;
		}
		return PUNCTUATION;

	case '!':
		//"!="
		if (*m_bufferMoved == '=')
		{
			_string += *m_bufferMoved;
			if (!Forward())
				return ERROR;
		}
		return PUNCTUATION;

	case '@':
		//"!="
		_string = "the @ is illegal";
		return ERROR;

	case '&':
		//"&&" "&="
		if (*m_bufferMoved == '&' || *m_bufferMoved == '=')
		{
			_string += *m_bufferMoved;
			if (!Forward())
				return ERROR;
		}
		return PUNCTUATION;

	case '<':
		//"<=" "<<"
		if (*m_bufferMoved == '=' || *m_bufferMoved == '<')
		{
			_string += *m_bufferMoved;
			if (!Forward())
				return ERROR;
		}
		return PUNCTUATION;

	case '>':
		//">=" ">>"
		if (*m_bufferMoved == '=' || *m_bufferMoved == '>')
		{
			_string += *m_bufferMoved;
			if (!Forward())
				return ERROR;
		}
		return PUNCTUATION;

	case '=':
		//"=="
		if (*m_bufferMoved == '=')
		{
			_string += *m_bufferMoved;
			if (!Forward())
				return ERROR;
		}
		return PUNCTUATION;

	case '|':
		//"||" "|="
		if (*m_bufferMoved == '|' || *m_bufferMoved == '=')
		{
			_string += *m_bufferMoved;
			if (!Forward())
				return ERROR;
		}
		return PUNCTUATION;

	case ':':
		//"::"
		if (*m_bufferMoved == ':')
		{
			_string += *m_bufferMoved;
			if (!Forward())
				return ERROR;
		}
		return PUNCTUATION;

	case '\'':
		while (*m_bufferMoved != '\'')
		{
			if (*m_bufferMoved == '\\')
			{
				_string += *m_bufferMoved;
				if (!Forward())
					return ERROR;
				if (*m_bufferMoved == '\'')
				{
					_string += *m_bufferMoved;
					if (!Forward())
						return ERROR;
				}
				else if (*m_bufferMoved == '\\')
				{
					_string += *m_bufferMoved;
					if (!Forward())
						return ERROR;
				}
			}
			else
			{
				_string += *m_bufferMoved;
				if (!Forward())
					return ERROR;
			}
		}
		_string += *m_bufferMoved;
		if (!Forward())
			return ERROR;
		return LITERAL;

	case '"':
		while (*m_bufferMoved != '"')
		{
			if (*m_bufferMoved == '\\')
			{
				_string += *m_bufferMoved;
				if (!Forward())
					return ERROR;
				if (*m_bufferMoved == '"')
				{
					_string += *m_bufferMoved;
					if (!Forward())
						return ERROR;
				}
				else if (*m_bufferMoved == '\\')
				{
					_string += *m_bufferMoved;
					if (!Forward())
						return ERROR;
				}
			}
			else
			{
				_string += *m_bufferMoved;
				if (!Forward())
					return ERROR;
			}
		}
		_string += *m_bufferMoved;
		if (!Forward())
			return ERROR;
		return LITERAL;

	default:
		break;
	}

	return PUNCTUATION;
}


bool LexicalAnalysis::IsDecimalNumber(char _input)
{
	return '0' <= _input && _input <= '9';
}

bool LexicalAnalysis::IsHexadecimalNumber(char _input)
{
	return ('0' <= _input && _input <= '9') || ('a' <= _input && _input <= 'f') || ('A' <= _input && _input <= 'F');
}

bool LexicalAnalysis::IsDot(char _input)
{
	return _input == '.';
}

bool LexicalAnalysis::IsAlphaOrUnderline(char _input)
{
	return (('a' <= _input && _input <= 'z') || ('A' <= _input && _input <= 'Z')) || _input == '_';
}

bool LexicalAnalysis::IsAlphaOrNumberOrUnderline(char _input)
{
	return IsDecimalNumber(_input) || IsAlphaOrUnderline(_input);
}

bool LexicalAnalysis::IsNonsense(char _input)
{
	return _input == ' ' || _input == '\t' || _input == '\n' || _input == '\r';
}

bool LexicalAnalysis::IsEnd(char _input)
{
	return _input == '\0';
}