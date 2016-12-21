#include <stdio.h>
#include <stdarg.h>
/**
 * Fake ISO C99 sscanf.
 *
 * __isoc99_sscanf was added to glibc in version 2.7, while libCAENDigitizer is
 * compiled using glibc version >=2.7.  In order to link it using older glibc,
 * __isoc99_sscanf has to be defined here manually using the sscanf function in
 * gnu extension of ansi c, which exists in glibc version < 2.7. References:
 *
 * - http://www.linuxquestions.org/questions/programming-9/undefined-reference-to-%60__isoc99_sscanf'-873058/
 * - http://stackoverflow.com/questions/3660826/gcc-ld-create-a-new-libc-so-with-isoc99-sscanfglibc-2-7-symbol-from-glibc
 */
int isoc99_sscanf(const char *a, const char *b, va_list args)
{
   int i;
   va_list ap;
   va_copy(ap,args);
   i=sscanf(a,b,ap); // exists in gnu extension of ansi c
   va_end(ap);
   return i;
}
/**
 * Make isoc99_sscanf an alias for __isoc99_sscanf@@GLIBC_2.7.
 *
 * It replaces the function of a versioning script. Reference:
 * https://ftp.gnu.org/old-gnu/Manuals/ld-2.9.1/html_node/ld_25.html
 */
__asm__(".symver isoc99_sscanf,__isoc99_sscanf@@GLIBC_2.7");
