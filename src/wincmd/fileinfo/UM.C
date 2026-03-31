#include    <string.h>  /* memmove, strchr, strcmp, strcpy, strlen */
#include    <ctype.h>   /* isdigit, isalnum */
#include    <stdlib.h>  /* malloc, free */
#include    <setjmp.h>  /* for truncation exit */

#include    "um.h"

/*****************************************************************************/

#ifdef  __FLAT__
#define _ss
#define far
#define near
#endif

/*****************************************************************************/

char        UMIVFLAG[]      = {'$', '*', '$', '*', '$', '*'};
unsigned long   UMIVNUM         = 0x03000000L;
char        UMIVCOPYRIGHT[] = "UNMANGLER";

/*****************************************************************************/

static  jmp_buf jmpb;

/*****************************************************************************
 *  If the name is more than MAXNOTRUNC chars long, it was truncated.
 */
#define MAXNOTRUNC  256

/*****************************************************************************
 *  The max. combined size of all function argument types, and the address
 *  of the argument buffer (assumed to be on the stack).
 */
#define MAXARGSLEN  512

static  char    *   argBuffNext;
static  size_t      argBuffFree;

/*****************************************************************************/
static  int     outerClass;
static  int     isTemplateName;
static  int     CountTemplateName = 0;
/*****************************************************************************/
static  void    arg_type(char **src, char **dest, size_t *avail, char *outer);

/*****************************************************************************
 *  Convert operator identifier to symbol.
 */
static  void    convertThunkName(char *dst, char *src, int thunk)
{
    switch  (thunk)
    {
    case    1:
        // Constructor displacement thunk 
        strcpy(dst, "__vc1$thunk__["); dst += 14;

#ifndef __FLAT__
        *dst++ = *src++; *dst++ = ',';  // ThisSize
        *dst++ = *src++; *dst++ = ',';  // VptrSize
        *dst++ = *src++; *dst++ = ',';  // FptrSize
#endif

        while   (*src != '$')       // VptrOffset
            *dst++ = *src++;
        *dst++ = ',';
         src++;

        while   (*src != '$')       // VtblOffset
            *dst++ = *src++;
        *dst++ = ',';
         src++;

        while   (*src != '$')       // ThisOffset
            *dst++ = *src++;
        *dst++ = ']';
         src++;

        *dst   = 0;
        break;
    }
}

/*****************************************************************************
 *
 *  Convert operator identifier to symbol.
 */
static  char    *  translate_op(char *src)
{
    struct trans
    {
        char    *  name;
        char    *  symbol;
    };

    static struct trans table[] =
    {
        { "add" , "+" },    { "adr", "&" },     { "and" , "&" },
        { "asg", "=" },     { "land", "&&" },   { "lor" , "||" },
        { "call", "()" },   { "cmp" , "~" },    { "fnc" , "()" },
        { "dec", "--" },    { "dele", "delete" },   { "div" , "/" },
        { "eql" , "==" },   { "geq" , ">=" },   { "gtr" , ">" },
        { "inc", "++" },    { "ind" , "*" },    { "leq" , "<=" },
        { "lsh" , "<<" },   { "lss" , "<" },    { "mod" , "%" },
        { "mul" , "*" },    { "neq" , "!=" },   { "new" , "new" },
        { "not" , "!" },    { "or"  , "|" },    { "rand", "&=" },
        { "rdiv", "/=" },   { "rlsh", "<<=" },  { "rmin", "-=" },
        { "rmod", "%=" },   { "rmul", "*=" },   { "ror" , "|=" },
        { "rplu", "+=" },   { "rrsh", ">>=" },  { "rsh" , ">>" },
        { "rxor", "^=" },   { "sub" , "-" },    { "subs", "[]" },
        { "xor",  "^" },    { "arow", "->"},
        { "nwa", "new[]" }, { "dla", "delete[]" },
        { 0, 0 }
    };

    struct  trans * t;

    t = table;
    while   (t->name && strcmp(t->name, src))
        ++t;
    if  (t->name == 0)
        longjmp(jmpb, 1);   /* presumably truncated */

    return  t->symbol;
}


/*****************************************************************************
 *  Copy a class name into the specified buffer, returning the total
 *  class name length; the class could be both nested and template.
 */
static  size_t copyClassName(char *  destPtr, char *    className, size_t    avail)
{
    char    *  begOffs = (char *)destPtr;
    char    *  maxOffs = (char *)destPtr + avail;

    for (; *className; className++)
    {
        if  ((char *)destPtr >= maxOffs)
            longjmp(jmpb, 1);

        if  (*className == '@')
        {
            *destPtr++ = ':';
            *destPtr++ = ':';
        }
        else if (*className == '%')
        {
            // Template class name
            className++;

            // Copy template name first
            while   (*className != '$')
            {
                if  (*className == 0)
                    longjmp(jmpb, 1);

                *destPtr++ = *className++;
            }

            *destPtr++ = '<';
            while   ((*className == '$') || isdigit(*className))
            {
                int     argtp;

                isTemplateName = 1;

                if (*className == '$') className++;
                argtp = *className;

                avail = maxOffs - (char *)destPtr;
				
				// Modif FG
                if  (argtp == 't') // This is a type argument
                {
	                className++;
                    arg_type(&className, &destPtr, &avail, 0);
                }
				else
                {
                    char    *    destSave;
                    size_t      availSave;

                    // A value argument -- skip the type
                    destSave = destPtr;
                    availSave = avail;
					arg_type(&className, &destPtr, &avail, 0);

/*                    else if  (*className != '%' )  /// ?????????????
					{
	                    destPtr  =  destSave;
		                avail    = availSave;

						// Type must be followed by '$'
		                if  (*className != '$' )
			                longjmp(jmpb, 1);
				        className++;

	                    switch  (argtp)
		                {
			                char    *   classBeg;
	                    case    'g':
		                    *destPtr++ = '&';

                        //* Fall through ..... 
		                case    'i':
	                        while   (*className != '$' && *className != '%')
			                {
					            if  ((char *)destPtr >= maxOffs || *className == 0)
						            longjmp(jmpb, 1);
	                            *destPtr++ = *className++;
		                    }
			                break;

						case    'm':
							*destPtr++ = '&';

	                        // Find the '$' delimiter 
	
		                    classBeg = className;
			                while   (*className != '$')
				            {
					            if  ((char *)destPtr >= maxOffs || *className == 0)
						            longjmp(jmpb, 1);
	                            className++;
		                    }
							*className = 0;
							avail = maxOffs - (char *)destPtr;
							destPtr += copyClassName(destPtr, classBeg, avail);

							*destPtr++ = ':';
							*destPtr++ = ':';

							className++;

							while   (*className != '$' &&
								 *className != '%')
							{
								if  ((char *)destPtr >= maxOffs || *className == 0)
									longjmp(jmpb, 1);

								*destPtr++ = *className++;
							}
							break;

						default:
							// Malformed string, presumably truncated 
							longjmp(jmpb, 1);
						}
					} /**/
                }
                if  ((*className == '$') || isdigit(*className))
                    *destPtr++ = ',';
            }
            *destPtr++ = '>';
			if (className[0] != '%' || className[1] != '@' )			
				break; // Assume truncation
        }
        else
            *destPtr++ = *className;
    }
    *destPtr = 0;
    return  (char *)destPtr - begOffs;
}

/*****************************************************************************
 *
 *  Skip a template class name of the form '%name<args>%'. This is not
 *  as trivial as it may seem, as one of the arguments may involve a
 *  'nested' template class name. The parameter points to the initial
 *  '%' of the template name.
 */
char *  skipTemplateName(char *srcP)
{
    char    *   maxP = srcP + strlen(srcP);
	CountTemplateName = 0;
	do
	{			
		while  (*++srcP != '%')
		{
			if  (srcP[0] == 0)
				return  srcP;

			if  (srcP[0] == '$' && srcP[1] == 't')
			{
				unsigned    len = 0;

				srcP += 2;

				while   (isdigit(*srcP))
				{
					len = len * 10 + *srcP++ - '0';
				}

				srcP += len - 1;
			}
		}
		if (*(srcP - 1) == '@') CountTemplateName++;
		else CountTemplateName--;
	} while (CountTemplateName > 0);
    return  (srcP >= maxP) ? maxP : srcP + 1;
}

/*****************************************************************************
 *
 *  Extract the class name from src, move it one character to the left
 *  to make room for a null terminator.  We know the first char is a digit.
 *  Return pointer to the name through src, and the name length.
 */
static  void    class_name(char **src, size_t *len)
{
    size_t  l;
    size_t  i = 0;
    char    *s = *src;

    do                  // compute length
    {
        i = i * 10 + *s++ - '0';
    }
        while(isdigit(*s));

    // guard against truncated name
    if  (strlen(s) < i)
        longjmp(jmpb, 1);

    --s;						// back up over last digit
    memmove(s, s+1, i);         // NOTE: overlapping move 
    s[i] = '\0';  
    *src = s; 

    // Nested class names take up a little more space
	// FG Template 
    for (l = i; l; s++, l--)
    {
		if ((*s == '@')) // || (*s == '%'))
			i++;
    }
	if (**src == '%') i+=2;
    *len = i;
}

// forward declaration 
static  void    arg_list(char **src, char **dest, size_t *avail);


/*****************************************************************************
 *  Interpret an isolated encoded argument type.
 */
static  void    arg_type(char **src, char **dest, size_t *avail, char *outer)
{
    char    *   p;
    char    *   s = *src;
    char    *   d = *dest;
    size_t      len;
    char    *   spec_ptr = 0;

    for (;;)                /* emit type qualifiers */
    {
// add - FG ? a voir
		if  (strncmp(s, "iuc",3) == 0)
		{
			s++; s++; s++;
			strcpy(d, "(uchar) ");
			d += 8; *avail -= 8;
			if (*s != '$') break;
			s++;
			while(isdigit(*s))
			{
				*d = *s;
				d++; s++;
				*avail--; 
			}
			goto DONE;
		} /**/
// add - FG ? a voir

        switch  (*s)
        {
		case 'u':
            if  (s[1] == 'p')
                spec_ptr = " huge ";
            else if (s[1] == 'r')
                spec_ptr = " _seg ";
            else
                p = "unsigned ";
            break;

        case 'z':
            p = "";
            break;

        case 'x':
            p = "const ";
            break;

        case 'w':
            p = "volatile ";
            break;

        default:
            goto    DONE_QUALS;
        }

        s++;                /* skip qualifier char */

        if  (!spec_ptr)
        {
            len = strlen(p);
            if  (*avail >= len)
           {
                strcpy(d, p);
                d += len;
                *avail -= len;
            }
            else
                *avail = 0;
            *d = 0;
        }
    }

DONE_QUALS:
    switch  (*s)                // check for built-in type
    {
		case 'o': p = "bool";		break;  
        case 'v': p = "void";       break;
        case 'c': p = "char";       break;
        case 'b': p = "wchar_t";    break;
        case 's': p = "short";      break;
        case 'i': p = "int";        break;
        case 'l': p = "long";       break;
        case 'f': p = "float";      break;
        case 'd': p = "double";     break;
        case 'g': p = "long double";    break;
        case 'e': p = "...";        break;
        default:
            goto    NOT_BUILT_IN;
    }
    len = strlen(p);
    if  (*avail >= len)
    {
        strcpy(d, p);
        d += len;
        *avail -= len;
    }
    else
        *avail = 0;
    s++;

    goto    DONE;

NOT_BUILT_IN:

    if  (isdigit(*s))           /* enum or class name */
    {
        class_name(&s, &len);

        if  (*avail >= len)
        {
            size_t      len;

            len = copyClassName(d, s, *avail);
            d += len;
            *avail -= len;
        }
        else
            *avail = 0;

        s += len;
    }
    else if (*s == 'p' || *s == 'r' || *s == 'm' || *s == 'n')
    {
        // ptr or ref to type
        short       is_func;
        short       is_ref = 0;
        size_t      len;

        char        cfunc;      // it is a const function 
        char        vfunc;      // it is a volatile function 

        char    *   p;

        if  (!spec_ptr)
        {
            is_ref = (*s == 'r' || *s == 'm');
			spec_ptr = " ";
/*          if  (*s == 'p' || *s == 'r')
                spec_ptr = " near";
            else
                spec_ptr = " far"; /**/
        }

        // look-ahead to see if this is a const/volatile function
        p = ++s;
        cfunc = vfunc = 0;

        for (;;)
        {
            if  (*p == 'x')
                cfunc = 1;
            else if (*p == 'w')
                vfunc = 1;
            else
                break;
            ++p;
        }

        is_func = 0;

        if  (*p == 'q')
        {
            is_func++;      // if not, ignore cfunc/vfunc
            spec_ptr++;     // omit leading blank 
            s = p;
        }

        arg_type(&s, &d, avail, spec_ptr);

        if  (is_func)
        {
            if  (cfunc && *avail > 6)
            {
                strcpy(d, " const");
                d      += 6;
                *avail -= 6;
            }

            if  (vfunc && *avail > 9)
            {
                strcpy(d, " volatile");
                d      += 9;
                *avail -= 9;
            }
        }
        else
        {
            if  (! isalnum(d[-1]))
                ++spec_ptr; /* omit leading blank */

            len = strlen(spec_ptr);
            if  (*avail > len)
            {
                strcpy(d, spec_ptr);
                d += len;
                *d++ = is_ref ? '&' : '*';
                *avail -= len + 1;
            }
        }
    }
    else if (*s == 'a')         /* array of type */
    {
        char dims[90];
        unsigned i = 0;
        do
        {
            dims[i++] = '[';
            if  (*++s == '0')
                ++s;				// 0 size means unpsecified
            while(*s && *s != '$')  // collect size, up to '$'
                dims[i++] = *s++;
            if (*s) ++s;
                dims[i++] = ']';
        }
            while (*s == 'a');  // collect all dimensions 
        dims[i] = '\0';
        arg_type(&s, &d, avail, 0);
        if  (*avail >= i + 2)
        {
            strcpy(d, dims);
            d += i;
            *avail -= i;
        }
        else if (*avail >= 2)
        {
            *d++ = '[';
            *d++ = ']';
            *d = '\0';
            *avail -= 2;
        }
        else
            *avail = 0;
    }
    else if (*s == 'q')         // function type
    {
        /*  We want the return type first, but find it last.
         *  So we emit all but the return type, get the return
         *  type, then shuffle to get them in the right place.
         */
        char    *start = d;

        ++s;
        if  (*avail >= 3)
        {
            *d++ = '(';
            *avail -= 1;

            if  (outer)
            {
                if  (outerClass)
                {
                    len = copyClassName(d, outer, *avail);

                    d      += len;
                    *avail -= len;

                    if  (*avail >= 2)
                    {
                        strcpy(d, "::");
                        d      += 2;
                        *avail -= 2;
                    }
                }
                else
                {
                    size_t      len = strlen(outer);

                    if  (*avail >= len+2)
                    {
                        strcpy(d, outer);
                        d      += len;
                        *avail -= len;
                    }
                }
            }

            *d++ = '*';
            *d++ = ')';
            *avail -= 2;
            *d = '\0';
        }
        else
            *avail = 0;
        arg_list(&s, &d, avail);
        if  (*s == '$')     /* flags the return type */
        {
            size_t      ret_len;
            char    *   ret_type;

            ++s;
            ret_type = d;
            arg_type(&s, &d, avail, 0); /* return type */
            ret_len = strlen(ret_type);
            if  (ret_len < 64)      /* check length */
            {
                char        ret_buff[64];

                strcpy(ret_buff, ret_type);
                memmove(start + ret_len, start, ret_type-start);
                memmove(start, ret_buff, ret_len);
            }
        }
    }
    else if (*s == 'M')         /* member pointer type */
    {
        char    *   classPtr;
        size_t      classLen;

        int     memberFn;
        char    *   outerPtr;

        /* Extract and save the position of the class name */
        classPtr = ++s;
        class_name(&classPtr, &classLen);
        s += classLen + 1;

        /* Is this a pointer to member function? */
        memberFn = 0;
        outerPtr = 0;

        if  (*s == 'q')
        {
            memberFn++;

            /* Use 'class::' for the 'outer' parameter */
            outerPtr   = classPtr;
            outerClass = 1;
        }

        /* Now decode the 'pointed-to' type */
        arg_type(&s, &d, avail, outerPtr);

        outerClass = 0;

        if  (!memberFn)
        {
            if  (*avail)
            {
                *d++ = ' ';
                *avail -= 2;
            }

            len =  copyClassName(d, classPtr, *avail);

            d      += len;
            *avail -= len;

            if  (*avail >= 3)
            {
                strcpy(d, "::*");
                d      += 3;
                *avail -= 3;
            }
        }
    }
    else if (*s)
    {
        longjmp(jmpb, 1);   /* malformed string, presumably truncated */
    }

DONE:

    *d = '\0';
    *src = s;
    *dest = d;
    return;
}


/*****************************************************************************
 *
 *  Process an argument list.
 */
static  void    arg_list(char **srcPP, char **dstPP, size_t *availP)
{
    char    *   srcP;
    char    *   dstP;
    size_t      avail;

    size_t      len;            /* temp string length */
    unsigned    i;          /* counter */

    /*
        We have to save up copies of the strings for each argument
        in case any are referenced with a "Tn" code.  There can be
        at most 35 of these.  Entry n in the array is copy of the
        string for the nth arg.
    */
    unsigned    argcount;
    char    *   argtypes[36];

    size_t      argBuffFreeSave;
    char    *   argBuffNextSave;

    /* Copy incoming parameters into local variables */

    srcP  = *srcPP;
    dstP  = *dstPP;
    avail = *availP;

    argcount = 0;
    argBuffFreeSave = argBuffFree;
    argBuffNextSave = argBuffNext;

    if  (avail > 0)
    {
        *dstP++ = '(';
        avail--;
        *dstP = 0;
    }

	if  ((*srcP == 'q') || (*(srcP+1) == 'r'))
	{
		srcP++; srcP++; 
	}

    if  (*srcP == 'v')      /* special case -- no parameters */
    {
        ++srcP;
        goto DONE;
    }

    for (;;)
    {
        argcount++;
        argtypes[argcount] = 0;
        if  (*srcP == 't')      /* repeat a previous type string */
        {
            srcP++;
            if  (isdigit(*srcP))
                i = *srcP - '0';
            else if (islower(*srcP))
                i = *srcP - 'a' + 10;
            else
                longjmp(jmpb, 1);   // assume truncation error 

            if  (i > argcount)
                longjmp(jmpb, 1);   // assume truncation error 

            if  (*srcP)
                srcP++;
            if  (argtypes[i])
            {
                len = strlen(argtypes[i]);
                if  (avail >= len)
                {
                    strcpy(dstP, argtypes[i]);
                    dstP  += len;
                    avail -= len;
                }
                else
                    avail = 0;
                }
            else
                longjmp(jmpb, 1);   // assume truncation error
        }
        else
        {
            char    *   dd;
            size_t      len;

            dd = dstP;
            arg_type(&srcP, &dstP, &avail, 0);

            len = strlen(dd) + 1;
            if  (argBuffFree >= len)
            {
                argtypes[argcount] = argBuffNext;
                strcpy(argBuffNext, dd);
                argBuffNext += len;
                argBuffFree -= len;
            }
        }

        // See if more arguments present
        if  (!isalnum(*srcP))
            break;

        if  (avail > 0)
        {
            *dstP++ = ',';
            *dstP = 0;
            avail--;
        }
    }

DONE:
    if  (avail > 0)
    {
        *dstP++ = ')';
        avail--;
    }
    *dstP = 0;

    argBuffFree = argBuffFreeSave;
    argBuffNext = argBuffNextSave;

    *srcPP  = srcP;
    *dstPP  = dstP;
    *availP = avail;
}

/*****************************************************************************
 *
 *  Given a potentially nested class name "name", return a pointer
 *  to the 'final' class name in the nested class name.
 *
 *  lastClassName("foo"     )   returns "foo"
 *  lastClassName("foo::bar")   returns "bar"
 *
 */
static  char *  lastClassName(char *name)
{
    char *      temp;

    for (temp = name; *temp; temp++)
    {
        if  (temp[0] == ':' && temp[1] == ':')
            name = temp + 2;
    }

    return  name;
}

/*****************************************************************************
 *
 *  Given a mangled name in "src", unmangle it if necessary, putting the
 *  result in "dest", copying at most "maxlen" characters, including
 *  trailing null.  It is assumed that dest is bigger than src, and big
 *  enough for any function name + class name.
 *
 *  Returns zero if the name is not a legal mangled name.
 *
 */
umKind  unmangle(char   *   src,
         char   *   dest,
         unsigned   maxlen,
         char   *   classP,
         char   *   nameP,
         int        doArgs)
{
    char    *   srcP;           // for scanning src 
    char    *   dstP;           // for scanning dest

    char    *   mainName;       // the main name
    char    *   className;      // qualifying name, if present

    char    *   dstClass;

    size_t      len;
    size_t      avail=0;			// chars left in dest

    char        cfunc;          // it is a const function
    char        vfunc;          // it is a volatile function

    char        argBuff[MAXARGSLEN];    // for arg tables
    char        name[MAXNOTRUNC];

    umKind      kind;
    int     thunk;

    if  (src == 0)
        return  UM_NOT_MANGLED;

    if  (*src != '@')
    {
        strcpy(dest, src);
        return  UM_NOT_MANGLED;
    }

    if  (dest)
    {
        char    *  tmpp = dest;
        char    *  endp = (char *)tmpp + maxlen;
        do
        {
            *tmpp++ = 0;
        }
        while   ((char *)tmpp < endp);
    }

    outerClass = 0;
	CountTemplateName = 0; 
    
	strncpy(name, src, MAXNOTRUNC-1); name[MAXNOTRUNC-1] = 0;

    // Setup arg buffer variables
    argBuffNext = argBuff;
    argBuffFree = MAXARGSLEN;

    // See if this is a pascal (up-cased) mangled name 
    for (srcP = name+1; *srcP; srcP++)
    {
        if  (*srcP >= 'a' && *srcP <= 'z')
            goto NOT_PASCAL;
    }

    // Entire name is uppercase, assume it's been pascal-ized 
    for (srcP = name+1; *srcP; srcP++)
        *srcP = tolower(*srcP);

NOT_PASCAL:

// Find the second '@' or '$' separator 
    srcP = name + 1;
    if  (srcP[0] == '$' && (srcP[1] == 'b' || srcP[1] == 'o'))   
		srcP += 3; // 3

	if (*srcP == '@')   //   __linkproc__
	{
		strncpy(name, src + 1, MAXNOTRUNC-1);
		strcpy(dest, "__linkproc__ ");
		avail -= 13;
	}	

	while   (*srcP)
    {
        if  (*srcP == '@' || *srcP == '$')
            break;

        if  (*srcP == '%')
        {
            srcP = skipTemplateName(srcP);
            continue;
        }

        srcP++;
    }

    // Can't be mangled if second '@' or '$' is missing
    if  (*srcP == 0)
    {
        // Special check: truncated template class name 
        if  (src[0] == '@' && src[1] == '%')
        {
            isTemplateName = 0;

            if  (!setjmp(jmpb))
            {
                if  (dest )
                    copyClassName(dest,  src+1, maxlen);
                if  (nameP)
                    copyClassName(nameP, src+1, maxlen);

                return UM_NOT_MANGLED;
            }

            // Is this a template name?
            if  (isTemplateName)
            {
                if  (dest )
                    strcat(dest,  "...>");
                if  (nameP)
                    strcat(nameP, "...>");

                return UM_NOT_MANGLED;
            }
        }

    NOT_MANGLED:

        if  (dest )
            strcpy(dest,  src);
        if  (nameP)
            strcpy(nameP, src);

        return UM_NOT_MANGLED;
    }

    // See if this is a member function
    if  (*srcP == '@')
    {		
        char    *   tmpP;	
        // See if this is a nested class member
        for (tmpP = srcP + 1; *tmpP; tmpP++)
        {
            if  (*tmpP == '@')
            {   // Remember last '@' separator 
                srcP = tmpP;
            }
            else if (*tmpP == '$')
            {
                break;
            }
            else if (*tmpP == '%')
            {// This is a template class name 
                srcP = skipTemplateName(tmpP);
            }
        }

        *srcP++ = 0;            // null-terminate class name
        className = name + 1; //adv;

        while   (isdigit(*srcP))    // 'huge' class? 
            srcP++;

        // Now pointing to the main name/
        mainName = srcP;

        // find '$' separator
        if  (srcP[0] == '$' && (srcP[1] == 'b' || srcP[1] == 'o'))
            srcP += 3;

        while   (*srcP && *srcP != '$')
            ++srcP;

        if  (*srcP == 0)
        {
            // Must be a static member
            kind = UM_STATIC_DM;
            goto COPYIT;
        }
    }
// test type descriptor 
	else if  ((*srcP == '$')  && (*(srcP+1) == 'x'))
	{
        char *tmpP;
		srcP += 4;
// See if this is a nested class member
        for (tmpP = srcP + 1; *tmpP; tmpP++)
        {
            if  (*tmpP == '@')
            {   // Remember last '@' separator
                srcP = tmpP;
            }
            else if (*tmpP == '$')
            {
                break;
            }
            else if (*tmpP == '%')
            {   // This is a template class name 
                srcP = skipTemplateName(tmpP);
            }
        }

        *srcP++ = 0;            // null-terminate class name
		className = name + 5;
		while (isdigit(*className)) className++;
		if (*className == '\0') 
		{
			className = NULL;
			srcP++;
		}
       // Now pointing to the main name 
        mainName = srcP;

        // find '$' separator
        if  (srcP[0] == '$' && (srcP[1] == 'b' || srcP[1] == 'o'))
            srcP += 3;
		
		strcpy(dest, "__tpdsc__ ");
		avail -= 10;
		kind = UM_TYPEDESCR;
        goto COPYIT;
	}
	else
    {
        className = 0;          // not a member function 
        mainName = name + 1;
		if  (mainName == srcP)
        {
            // 'main' name appears to be empty - is this a thunk?
            if  (strncmp(srcP, "$vc1$", 5))
                goto NOT_MANGLED;

            srcP += 5;
            kind  = UM_THUNK;
            thunk = 1;

            goto COPYIT;
        }
    }

    /* null-terminate "main" name and point to encoding of type definition */
    *srcP++ = 0;

    /* Now figure out what kind of beast this is */
    if  (mainName[0] == '$')        /* check for special names */
    {
        mainName += 2;

        if  (mainName[-1] == 'b')   /* ctor, dtor, or operator */
        {
            if  (strcmp(mainName, "ctr") == 0)
                kind = UM_CONSTRUCTOR;
            else if (strcmp(mainName, "dtr") == 0)
                kind = UM_DESTRUCTOR;
            else
                kind = UM_OPERATOR;
        }
        else
            kind = UM_CONVERSION;   // type conversion
    }
    else
        kind = UM_MEMBER_FN;        // "ordinary" name

    // Copy class and main name to caller's buffers (if applicable)
COPYIT:

    dstClass = 0;

    if  (classP)
        copyClassName(classP, className, maxlen);

    if  ( nameP)
    {
        if  (dest == 0)
        {
            dstP   = nameP;
            doArgs = 0;
            avail  = 63;

            if  (setjmp(jmpb))
                goto trunc_exit;

            goto DONAME;
        }

        strcpy( nameP,  mainName);
    }

    if  (dest == 0)
        return  kind;

    // Setup destination pointer, and remaining dest. buffer length
    dstP  = dest;
	dstP  += strlen( dest );
    {   // Leave some extra room at the end
        size_t used = (size_t)(dstP - dest);
        avail = (maxlen > used + 5) ? maxlen - used - 5 : 0;
    }

    // Prepare the quick exit route
    if  (setjmp(jmpb))
    {
trunc_exit:
        // Make sure there's room for the "..." 
        if  (maxlen)
            dest[maxlen - 4] = 0;

        strcat(dstP, "...");
        return  kind;
    }

    // Setup arg buffer variables
    argBuffNext = argBuff;
    argBuffFree = MAXARGSLEN;

    // emit qualifying name, if any
#ifdef  IN_MOJO
    if  (className && !(doArgs & DoArgsNoClassPrefix))
#else
    if  (className)
#endif
    {
        dstClass = dstP;

        len  = copyClassName(dstP, className, avail);
        dstP   += len;
        *dstP++ = ':';
        *dstP++ = ':';
        avail   -= len + 2;
    }

    // emit the main name
DONAME:

    switch  (kind)
    {
    case    UM_DESTRUCTOR:
        *dstP++ = '~';

    case    UM_CONSTRUCTOR:
        if  (dstClass)
        {
            char    *   temp = dstP - 2;

            if  (temp[-1] == ':')
                temp--;

            *temp = 0;
            strcpy(dstP, lastClassName(dstClass));
            dstP += strlen(dstP);
            *temp = ':';
        }
        else
        {
            dstP += copyClassName(dstP, className, avail);
        }
        break;

    case    UM_OPERATOR:
        strcpy(dstP, "operator ");
        dstP += 9;
        strcpy(dstP, translate_op(mainName));
        break;

    case    UM_CONVERSION:
        strcpy(dstP, "operator ");
        dstP  += 9;
        avail -= 9;
        arg_type(&mainName, &dstP, &avail, 0);
        kind = UM_CONVERSION;
        break;

    case    UM_MEMBER_FN:
    case    UM_STATIC_DM:
        strcpy(dstP, mainName);
		if (src[strlen(src)-1] == '@')		
			kind = UM_VTABLE;
        break;

    case    UM_TYPEDESCR:
        strcpy(dstP, mainName);
        break;
    case    UM_THUNK:
        convertThunkName(dstP, srcP, thunk);
        break;
    }

#ifdef  IN_MOJO
    if  (!(doArgs & DoArgsArguments))
        return  kind;
#else
    if  (!doArgs)
        return  kind;
#endif

    // Point to end of destination and figure available space 
    dstP += strlen(dstP);
    avail = dest + maxlen - dstP - 1;
    if  (avail < 0)
        avail = 0;

    // check for a const/volatile function 
    cfunc = vfunc = 0;
    for (;;)
    {
        if  (*srcP == 'x')
            cfunc = 1;
        else if (*srcP == 'w')
            vfunc = 1;
        else
            break;
        ++srcP;
    }

    // handle argument list
    if  (*srcP == 'q')
    {
        ++srcP;
        arg_list(&srcP, &dstP, &avail);
    }

    // tack on const/volatile function modifier 
    if  (cfunc)
    {
        if  (avail < 6)
            goto trunc_exit;
        strcat(dstP, " const");
    }

    if  (vfunc)
    {
        if  (avail < 9)
            goto trunc_exit;
        strcat(dstP, " volatile");
    }

    return  kind;
}

#ifdef  TEST

int main(int argc, char *argv[])
{
    char    buff[512];
    int i;

    if  (argc == 1)
    {
        argc    = 2;
        argv[1] = "@foo@bar@$bctr$q";
    }

    for (i = 1; i < argc; i++)
    {
 		printf("%s ", argv[i]);

        switch  (unmangle(argv[i], buff, 512, 0, 0, 1))
        {
        case    UM_NOT_MANGLED:
            printf(" -> not mangled %s\n", buff);
            break;

        case    UM_MEMBER_FN:
            printf(" -> Function   %s\n", buff);
            break;

        case    UM_CONSTRUCTOR:
            printf(" -> Constructor %s\n", buff);
            break;

        case    UM_DESTRUCTOR:
            printf(" -> Destructor  %s\n", buff);
            break;

        case    UM_OPERATOR:
            printf(" -> Operator    %s\n", buff);
            break;

        case    UM_CONVERSION:
            printf(" -> Conversion  %s\n", buff);
            break;

        case    UM_STATIC_DM:
            printf(" -> Static data %s\n", buff);
            break;

        case    UM_THUNK:
            printf(" -> Thunk       '%s'\n", buff);
            break;
        case    UM_TYPEDESCR:
            printf(" -> Type Descriptor '%s'\n", buff);
            break;
        case    UM_VTABLE:
            printf(" -> VTable          '%s'\n", buff);
            break;
		case    UM_OTHER:
            printf(" -> ???????????     '%s'\n", buff);
            break;

        }
    }
    return  0;
}

#endif
// @Inpdlg@THPGInputDlg@ControlKeyDown$qqrp14System@TObjectrus46System@%Set$t18Classes@Classes__1$iuc$0$iuc$6%
// $iuc$0$iuc$6% =>     int, (uchar) 0, int, (uchar) 6)
