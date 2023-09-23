// #include <stdarg.h>
#include <asm/div64.h>
#include <asm/stdio.h>
#include <asm/types.h>
#include <linux/ctype.h>
#include <linux/string.h>

/**
 * 将字符串转换为无符号长整型数。
 *
 * @cp: 指向输入字符串的指针。
 * @endp: 如果不为NULL，则在转换完成后，此指针指向字符串中的下一个未处理字符。
 * @base:
 * 要解析的数字的基数(例如，10代表十进制)。如果为0，则根据字符串的前缀决定基数。
 *
 * @return: 返回转换得到的无符号长整数。
 */
unsigned long simple_strtoul(const char *cp, char **endp, unsigned int base) {
  unsigned long result = 0, value;
  // 如果base为0，则根据字符串前缀自动决定基数
  if (!base) {
    base = 10;
    if (*cp == '0') {
      base = 8;
      cp++;
      if ((*cp == 'x') && isxdigit(cp[1])) {
        cp++;
        base = 16;
      }
    }
  }
  // 当前字符是合法的数字，并且它的值小于基数时，继续转换
  while (isxdigit(*cp) &&
         (value = isdigit(*cp) ? *cp - '0' : toupper(*cp) - 'A' + 10) < base) {
    result = result * base + value;
    cp++;
  }
  // 如果endp不为NULL，则设置它指向未处理的字符
  if (endp) *endp = (char *)cp;  // 返回转换得到的值
  return result;
}

/**
 * simple_strtol - 将字符串转换为长整型数值
 * @cp: 要转换的字符串指针
 * @endp: 一个指向指针的指针，用于存储转换结束的位置
 * @base: 转换的基数
 */
long simple_strtol(const char *cp, char **endp, unsigned int base) {
  if (*cp == '-') return -simple_strtoul(cp + 1, endp, base);
  return simple_strtoul(cp, endp, base);
}

/**
 * 将字符串转换为无符号长长整型数。
 *
 * @cp: 指向输入字符串的指针。
 * @endp: 如果不为NULL，则在转换完成后，此指针指向字符串中的下一个未处理字符。
 * @base:
 * 要解析的数字的基数(例如，10代表十进制)。如果为0，则根据字符串的前缀决定基数。
 *
 * @return: 返回转换得到的无符号长长整数。
 */
unsigned long long simple_strtoull(const char *cp, char **endp,
                                   unsigned int base) {
  unsigned long long result = 0, value;

  // 如果base为0，则根据字符串前缀自动决定基数
  if (!base) {
    base = 10;
    if (*cp == '0') {
      base = 8;
      cp++;
      if ((*cp == 'x') && isxdigit(cp[1])) {
        cp++;
        base = 16;
      }
    }
  }
  // 当前字符是合法的数字，并且它的值小于基数时，继续转换
  while (isxdigit(*cp) &&
         (value = isdigit(*cp) ? *cp - '0'
                               : (islower(*cp) ? toupper(*cp) : *cp) - 'A' +
                                     10) < base) {
    result = result * base + value;
    cp++;
  }
  if (endp) *endp = (char *)cp;
  return result;
}

/**
 * simple_strtoll - 将字符串转换为长长整型数值
 * @cp: 要转换的字符串指针
 * @endp: 一个指向指针的指针，用于存储转换结束的位置
 * @base: 转换的基数
 */
long long simple_strtoll(const char *cp, char **endp, unsigned int base) {
  if (*cp == '-') return -simple_strtoull(cp + 1, endp, base);
  return simple_strtoull(cp, endp, base);
}

/**
 * skip_atoi - 跳过字符串中的数字并返回对应的整数值
 * @s: 字符串指针的指针，通过引用传递以更新指针位置
 *
 * 该函数跳过字符串中的数字字符，并返回所跳过的数字对应的整数值。
 * 函数使用指针的指针来处理字符串的位置，通过指针传递以更新指针位置。
 */
static int skip_atoi(const char **s) {
  int i = 0;

  while (isdigit(**s)) i = i * 10 + *((*s)++) - '0';
  return i;
}

#define ZEROPAD 1 /* pad with zero */      // 使用零填充
#define SIGN 2 /* unsigned/signed long */  // 无符号/有符号长整数
#define PLUS 4 /* show plus */             // 显示加号
#define SPACE 8 /* space if plus */        // 如果有加号，则显示空格
#define LEFT 16 /* left justified */       // 左对齐
#define SPECIAL 32 /* 0x */                // 0x前缀
#define LARGE \
  64 /* use 'ABCDEF' instead of 'abcdef' */  // 使用大写字母'ABCDEF'而不是小写字母'abcdef'

/**
 * 将数字转换为字符串
 * @str: 目标字符串的指针，用于存储转换后的结果
 * @num: 待转换的数值
 * @base: 指定的进制，取值范围为2到36
 * @size: 目标字符串的总长度，包括数值本身和可能的填充字符
 * @precision: 数值的精度，即转换后的字符串中数值部分的最小长度
 * @type: 转换的类型和选项，用位标志表示
 *
 * 该函数接受一个数值num，并根据指定的进制base将其转换为字符串形式，然后存储到字符数组str中。
 *
 * @return: 指向目标字符串末尾的指针
 */
static char *number(char *str, long long num, int base, int size, int precision,
                    int type) {
  char c, sign, tmp[66];
  const char *digits = "0123456789abcdefghijklmnopqrstuvwxyz";
  int i;

  if (type & LARGE)  // 如果使用大写字母，则使用大写字母的数字字符集
    digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  if (type & LEFT)  // 如果是左对齐，则取消使用零填充的标志
    type &= ~ZEROPAD;
  if (base < 2 || base > 36)  // 如果进制不在2到36之间，则返回错误
    return 0;
  c = (type & ZEROPAD) ? '0'
                       : ' ';  // 如果使用零填充，则填充字符为'0'，否则为' '
  sign = 0;                    // 符号位初始化为0
  if (type & SIGN) {
    if (num < 0) {
      sign = '-';
      num = -num;
      size--;
    } else if (type & PLUS) {
      sign = '+';
      size--;
    } else if (type & SPACE) {
      sign = ' ';
      size--;
    }
  }
  if (type & SPECIAL) {
    if (base == 16)
      size -= 2;
    else if (base == 8)
      size--;
  }
  i = 0;
  if (num == 0)  // 如果数值为0，则直接将'0'存入临时字符数组
    tmp[i++] = '0';
  else
    while (num != 0)  // 将数值按给定进制转换为字符，并存入临时字符数组
      tmp[i++] = digits[do_div(num, base)];
  if (i >
      precision)  // 如果实际转换得到的字符数大于指定的精度，则将精度设置为实际字符数
    precision = i;
  size -= precision;  // 计算剩余可用空间
  if (!(type & (ZEROPAD +
                LEFT)))  // 如果不是左对齐且不使用零填充，则在剩余空间前填充空格
    while (size-- > 0) *str++ = ' ';
  if (sign) *str++ = sign;  // 存储符号位
  if (type & SPECIAL) {
    if (base == 8)  // 如果是8进制数，则存储前缀"0"
      *str++ = '0';
    else if (base == 16) {  // 如果是8进制数，则存储前缀"0"
      *str++ = '0';
      *str++ = digits[33];
    }
  }
  if (!(type & LEFT))
    while (size-- > 0)  // 在剩余空间前填充指定的字符
      *str++ = c;
  while (i < precision--)  // 在字符转换后的数值前填充零，以满足指定的精度
    *str++ = '0';
  while (i-- > 0)  // 将字符转换后的数值逆序存储到目标字符串中
    *str++ = tmp[i];
  while (size-- > 0)  // 在剩余空间后填充空格
    *str++ = ' ';
  return str;  // 返回指向目标字符串末尾的指针
}

/* Forward decl. needed for IP address printing stuff... */
int sprintf(char *buf, const char *fmt, ...);

int vsprintf(char *buf, const char *fmt, va_list args) {
  int len;
  unsigned long long num;
  int i, base;
  char *str;
  const char *s;

  int flags; /* flags to number() */

  int field_width; /* width of output field */  // 输出字段的宽度
  int precision; /* min. # of digits for integers; max			//
                    整数的最小位数；字符串的最大字符数 number of chars for from
                    string */
  int qualifier; /* 'h', 'l', or 'L' for integer fields */  // 用于整数字段的限定符（'h'、'l'、'L'、'Z'）
                                                            // short，long，long
                                                            // double，size_t
  /* 'z' support added 23/7/1999 S.H.    */
  /* 'z' changed to 'Z' --davidm 1/25/99 */

  for (str = buf; *fmt; ++fmt) {
    if (*fmt != '%') {  // 如果当前字符不是 '%'
      *str++ = *fmt;
      continue;
    }
    // 处理标志位
    /* process flags */
    flags = 0;
  repeat:
    ++fmt; /* this also skips first '%' */
    switch (*fmt) {
      case '-':
        flags |= LEFT;
        goto repeat;
      case '+':
        flags |= PLUS;
        goto repeat;
      case ' ':
        flags |= SPACE;
        goto repeat;
      case '#':
        flags |= SPECIAL;
        goto repeat;
      case '0':
        flags |= ZEROPAD;
        goto repeat;
    }
    // 获取字段宽度
    /* get field width */
    field_width = -1;
    if (isdigit(*fmt))
      field_width = skip_atoi(&fmt);  // 跳过字符串中的数字并返回对应的整数值
    else if (
        *fmt ==
        '*') {  // 宽度在format字符串中未指定，但是会作为附加整数值参数放置于要被格式化的参数之前
      ++fmt;
      /* it's the next argument */
      field_width = va_arg(args, int);  // 下一个参数是字段宽度
      if (field_width < 0) {
        field_width = -field_width;
        flags |= LEFT;
      }
    }
    // 获取精度
    /* get the precision */
    precision = -1;
    if (*fmt == '.') {
      ++fmt;
      if (isdigit(*fmt))
        precision = skip_atoi(&fmt);  // 跳过字符串中的数字并返回对应的整数值
      else if (
          *fmt ==
          '*') {  // 精度在format字符串中未指定，但是会作为附加整数值参数放置于要被格式化的参数之前
        ++fmt;
        /* it's the next argument */
        precision = va_arg(args, int);
      }
      if (precision < 0) precision = 0;
    }
    // 获取转换限定符
    /* get the conversion qualifier */
    qualifier = -1;
    if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L' || *fmt == 'Z') {
      qualifier = *fmt;
      ++fmt;
    }

    /* default base */
    base = 10;

    switch (*fmt) {
      case 'c':  // 字符
        if (!(flags & LEFT))
          while (--field_width > 0) *str++ = ' ';
        *str++ = (unsigned char)va_arg(args, int);
        while (--field_width > 0) *str++ = ' ';
        continue;

      case 's':  // 字符串
        s = va_arg(args, char *);
        if (!s) s = "<NULL>";

        len = strnlen(s, precision);

        if (!(flags & LEFT))
          while (len < field_width--) *str++ = ' ';
        for (i = 0; i < len; ++i) *str++ = *s++;
        while (len < field_width--) *str++ = ' ';
        continue;

      case 'p':  // 指针
        if (field_width == -1) {
          field_width = 2 * sizeof(void *);
          flags |= ZEROPAD;  // 补零标志
        }
        str = number(str, (unsigned long)va_arg(args, void *), 16, field_width,
                     precision, flags);  // 将指针转换为字符串
        continue;

      case 'n':  // 获取字符数
        if (qualifier == 'l') {
          long *ip = va_arg(args, long *);
          *ip = (str - buf);
        } else if (qualifier == 'Z') {
          size_t *ip = va_arg(args, size_t *);
          *ip = (str - buf);
        } else {
          int *ip = va_arg(args, int *);
          *ip = (str - buf);
        }
        continue;

      case '%':
        *str++ = '%';
        continue;

      /* integer number formats - set up the flags and "break" */
      case 'o':  // 八进制
        base = 8;
        break;

      case 'X':  // 十六进制大写
        flags |= LARGE;
      case 'x':  // 十六进制
        base = 16;
        break;

      case 'd':  // 十进制有符号整数
      case 'i':  // 整数
        flags |= SIGN;
      case 'u':  // 无符号整数
        break;

      default:  // 未知格式化字符
        *str++ =
            '%';  // 遇到未知的格式化字符，则将%字符和后面的字符原样复制到目标缓冲区中
        if (*fmt)
          *str++ = *fmt;
        else
          --fmt;
        continue;
    }
    if (qualifier == 'L')
      num = va_arg(args, long long);
    else if (qualifier == 'l') {
      num = va_arg(args, unsigned long);
      if (flags & SIGN) num = (signed long)num;
    } else if (qualifier == 'Z') {
      num = va_arg(args, size_t);
    } else if (qualifier == 'h') {
      num = (unsigned short)va_arg(args, int);
      if (flags & SIGN) num = (signed short)num;
    } else {
      num = va_arg(args, unsigned int);
      if (flags & SIGN) num = (signed int)num;
    }
    str = number(str, num, base, field_width, precision, flags);
  }
  *str = '\0';
  return str - buf;
}

int sprintf(char *buf, const char *fmt, ...) {
  va_list args;
  int i;

  va_start(args, fmt);
  i = vsprintf(buf, fmt, args);
  va_end(args);
  return i;
}
