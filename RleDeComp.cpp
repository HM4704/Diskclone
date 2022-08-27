#include "StdAfx.h"
#include <stdlib.h>
#include "RleDeComp.h"

CRleDeComp::CRleDeComp(void)
{
}

CRleDeComp::~CRleDeComp(void)
{
}

#if 1
#define RLE4K \
                    if (iUcnt <3) \
                    { \
                       if (iUcnt == 1) \
                       { \
                          if (byteflag == 0x00 ) \
                          { \
                             *pD=0x00; \
                          } \
                          else \
                          { \
                             pD++; \
                          } \
                          byteflag=~byteflag; \
                       } \
                       else if (iUcnt == 2) \
                       { \
                          if (byteflag == 0x00 ) \
                          { \
                             *pD=0x00; \
                             pD++; \
                          } \
                          else \
                          { \
                             pD++; \
                             *pD=0x00; \
                          } \
                       } \
                       iUcnt = 0; \
                    } \
                    else /* RLE abspeichern */ \
                    { \
                       if ( byteflag !=0x00 ) \
                       { \
                          *pD=(*pD)+0x09; \
                          *(pD+1)=((iUcnt-3)<<4); \
                          pD++; \
                       } \
                       else \
                       { \
                          *pD++= 0x90+(iUcnt-3); \
                       } \
                       iUcnt=0; \
                    }

int CRleDeComp::ZipDeltaRLE( BYTE *pSource, BYTE *pD, int nByteAnz )
{
   BYTE *pD0, iUcnt;
   int iScnt,difference,byteflag=0;
   pD0   = pD;
   iScnt = nByteAnz-1;
   iUcnt = 0;
   *pD  = 0x70+ (*(pSource)>>4) ;
   *(pD+1)  = (*(pSource)<<4);
   byteflag = ~byteflag;
   pD++;
   while (iScnt >0 )
   {
      iScnt--;
      pSource++;
      difference   = *(pSource)- *(pSource-1);
      if ( difference != 0x00 )
      {
         RLE4K
         if (abs(difference) <= 6 )
         {
            if ( difference <0 )
            {
               difference += 16;
            }
            if (byteflag == 0 )
            {
               *pD=difference<<4;
            }
            else
            {
               *pD = (*pD)+difference;
               pD++;
            }
         }
         else
         {
            if (abs(difference) <=13   )
            {
               if ( difference <0 )
               {
                  difference += 22;
               }
               else
               {
                  difference -= 7;
               }
               if (byteflag == 0)
               {
                  *pD = 0x80+difference;
                  pD++;
               }
               else
               {
                  *pD = (*pD)+0x08;
                  pD++;
                  *pD=difference<<4;
               }
               byteflag = ~byteflag;
            }
            else
            {
               {
                  if (byteflag == 0 )
                  {
                     *pD=0x70+ (*(pSource)>>4);
                     *(pD+1)=(*(pSource)<<4);
                     pD++;
                  }
                  else
                  {
                     *pD=(*pD)+0x07;
                     *(pD+1) = *(pSource);
                     pD+= 2;
                  }
               }
            }
         }
         byteflag = ~byteflag;
         iUcnt = 0;
      }
      else
      {
         if (iUcnt >=18 )
         {
            lab0:
            if ( byteflag != 0 )
            {
               *pD = (*pD)+0x09;
               *(pD+1) = 0x0f<<4;
               pD++;
            }
            else
            {
               *pD++= 0x90+0x0f;
            }
            if ((pSource == 0) && ((unsigned int)*((unsigned int*)pSource) == 0) && ((unsigned int)*((unsigned int*)pSource+1) == 0) && ((unsigned int)*((unsigned int*)pSource+2) == 0) && ((unsigned int)*((unsigned int*)pSource+3) == 0) && ((unsigned short)*((unsigned int*)pSource+4) == 0))
            {
               if (iScnt>18)
               {
                  pSource +=18;
                  iScnt -=18;
                  goto lab0;
               }
               else
               {
                  iUcnt = 1;
               }
            }
            else
            {
               iUcnt = 1;
            }
         }
         else
         {
            iUcnt++;
         }
      }
   }
   if (iUcnt >0 )
   {
      RLE4K
   }
   if ( byteflag != 0 )
   {
      *pD = (*pD)+0x08; 
       pD++;
      *pD = 0x70;
   }
   else
   {
      *pD = 0x87; 
   }
   return  ((int)(pD-pD0+1));
}
int CRleDeComp::DeZipDeltaRLE ( BYTE *pSource, BYTE *pD, int nByteAnz )
{
      BYTE H4bits, L4bits,*pD0,last;
      pD0   = pD;
      last = *pD;
   gerade:
   {
      if ((*pSource & 0xF0) == 0x90)
      {
#if(0)
         int i, iVal;
         unsigned long v;
#endif
         int iUcnt;
         L4bits = *pSource & 0x0F;
         iUcnt = L4bits+3;
#if 0
         while (iUcnt-->0)
         {
            *pD++= *(pD-1);
         }
#else
#if(1)
         memset(pD, last, 18);
         pD += iUcnt;
#else
         iVal = iUcnt;
         iVal /= 4;
         iVal++;
         v = last;
         v <<= 8;
         v |= last;
         v <<= 8;
         v |= last;
         v <<= 8;
         v |= last;
         i = 0;
         for (i; i < iVal; i++)
         {
            (unsigned long)*(((unsigned long*)pD)+i) = v;
         }
         pD += iUcnt;
#endif
#endif
         pSource++;
            goto gerade;
      }
      else
      {
         if ((*pSource & 0xF0) == 0x70)
         {
               L4bits = *pSource & 0x0F;
               H4bits= *(pSource+1) & 0xF0;
               H4bits >>= 4;
               L4bits <<= 4;
               *pD++= last = L4bits+H4bits;
            pSource++;
               goto ungerade;
         }
         else
         {
            if ((*pSource & 0xF0) == 0x80)
            {
               L4bits = *pSource & 0x0F;
               if (L4bits == 7)
               {
                  return  ((int)(pD-pD0));
               }
               else
               {
                  if (L4bits <=7)
                  {
                     L4bits+= 0x07;
                  }
                  else
                  {
                     L4bits+= 234;
                  }
               }
                  *pD++= last = last+L4bits;
               pSource++;
               goto gerade;
            }
            else
            {
               H4bits = *pSource & 0xF0;
               H4bits >>= 4;
               if (H4bits > 6)
               {
                  H4bits+=  240;
               }
                  *pD++= last = last+ H4bits;
               goto ungerade;
            }
         }
      }
   }
   ungerade:
   {
      if ((*pSource & 0x0F)==0x09)
      {
#if(0)
         int i, iVal;
         unsigned long v;
#endif
         int iUcnt;
         L4bits= *(pSource+1) & 0xF0;
            iUcnt= (L4bits>>4)+3;
#if 0
         while (iUcnt-->0)
         {
            *pD++= *(pD-1);
         }
#else
#if(1)
         memset(pD, last, 18);
         pD += iUcnt;
#else
         iVal = iUcnt;
         iVal /= 4;
         iVal++;
         v = last;
         v <<= 8;
         v |= last;
         v <<= 8;
         v |= last;
         v <<= 8;
         v |= last;
         i = 0;
         for (i; i < iVal; i++)
         {
            (unsigned long)*(((unsigned long*)pD)+i) = v;
         }
         pD += iUcnt;
#endif
#endif
         pSource++;
            goto ungerade;
      }
      else
      {
         if ((*pSource & 0x0F)==0x07)
         {
            *pD++= last = *(pSource+1);
               pSource+=2;
               goto gerade;
         }
         else
         {
            if ((*pSource & 0x0F)==0x08)
            {
               L4bits= *(pSource+1) & 0xF0;
               L4bits >>= 4;
               if (L4bits == 7)
               {
                  return  ((int)(pD-pD0));
               }
               else
               {
                  if (L4bits <= 7)
                  {
                     L4bits+= 0x07;
                  }
                  else
                  {
                     L4bits+= 234;
                  }
               }
                  *pD++= last = last+L4bits;
               pSource++;
               goto ungerade;
            }
            else
            {
               H4bits= *pSource & 0x0F;
               if (H4bits >6)
               {
                  H4bits+= 240;
               }
                  *pD++= last = last+H4bits;
               pSource++;
                  goto gerade;
            }
         }
      }
   }
   return  0;
}
#else
#define RLE4K \
                    if (iUcnt <3) \
                    { \
                       if (iUcnt == 1) \
                       { \
                          if (byteflag == 0x00 ) \
                          { \
                             *pD=0x00; \
                          } \
                          else \
                          { \
                             pD++; \
                          } \
                          byteflag=~byteflag; \
                       } \
                       else if (iUcnt == 2) \
                       { \
                          if (byteflag == 0x00 ) \
                          { \
                             *pD=0x00; \
                             pD++; \
                          } \
                          else \
                          { \
                             pD++; \
                             *pD=0x00; \
                          } \
                       } \
                       iUcnt = 0; \
                    } \
                    else /* RLE abspeichern */ \
                    { \
                       if ( byteflag !=0x00 ) \
                       { \
                          *pD=(*pD)+0x09; \
                          *(pD+1)=((iUcnt-3)<<4); \
                          pD++; \
                       } \
                       else \
                       { \
                          *pD++= 0x90+(iUcnt-3); \
                       } \
                       iUcnt=0; \
                    }


int CRleDeComp::ZipDeltaRLE( BYTE *pSource, BYTE *pD, int nByteAnz )
{
   BYTE *pD0, iUcnt;
   int iScnt,difference,byteflag=0;
   pD0   = pD;
   iScnt = nByteAnz-1;
   iUcnt = 0;
   *pD  = 0x70+ (*(pSource)>>4) ;
   *(pD+1)  = (*(pSource)<<4);
   byteflag = ~byteflag;
   pD++;
   while (iScnt >0 )
   {
      iScnt--;
      pSource++;
      difference   = *(pSource)- *(pSource-1);
      if ( difference != 0x00 )
      {
         RLE4K
         if (abs(difference) <= 6 )
         {
            if ( difference <0 )
            {
               difference += 16;
            }
            if (byteflag == 0 )
            {
               *pD=difference<<4;
            }
            else
            {
               *pD = (*pD)+difference;
               pD++;
            }
         }
         else
         {
            if (abs(difference) <=14   )
            {
               if ( difference <0 )
               {
                  difference += 22;
               }
               else
               {
                  difference -= 7;
               }
               if (byteflag == 0)
               {
                  *pD = 0x80+difference;
                  pD++;
               }
               else
               {
                  *pD = (*pD)+0x08;
                  pD++;
                  *pD=difference<<4;
               }
               byteflag = ~byteflag;

            }
            else /* neuer Startwert */
            {
               if (byteflag == 0 )
               {
                  *pD=0x70+ (*(pSource)>>4);
                  *(pD+1)=(*(pSource)<<4);
                  pD++;
               }
               else
               {
                  *pD=(*pD)+0x07;
                  *(pD+1) = *(pSource);
                  pD+= 2;
               }
            }
         }

         byteflag = ~byteflag;
         iUcnt = 0;
      }
      else
      {
         if (iUcnt >=18 )
         {
            if ( byteflag != 0 )
            {
               *pD = (*pD)+0x09;
               *(pD+1) = 0x0f<<4;
               pD++;
            }
            else
            {
               *pD++= 0x90+0x0f;
            }
            iUcnt = 1;
         }
         else
         {
            iUcnt++;
         }
      }
   }
   // rest abspeichern
   if (iUcnt >0 )
   {
      RLE4K
   }
   if ( byteflag != 0 )
   {
	   *pD = (*pD)+0x07; // neuer Startwert ist quasi Endekennung
   }
   else
   {
      pD-= 1;
   }
   return  ((int)(pD-pD0+1));
}

#if 1
int CRleDeComp::DeZipDeltaRLE ( BYTE *pSource, BYTE *pD, int nByteAnz )
{
   BYTE H4bits, L4bits,*pD0,iUcnt;
//   DWORD dwVal;
   int iScnt, byteflag = 0;
   iScnt = nByteAnz;
   pD0   = pD;

   goto byteflag_0;
byteflag_1:
   while ( iScnt > 0 )
   {
        iScnt--;

         switch (*pSource & 0x0F)
         {
            case 0x09:
            {
               L4bits= *(pSource+1) & 0xF0;
               L4bits>>= 4;
               iUcnt= L4bits+3;
#if 1
               while (iUcnt-->0)
               {
                  *pD++= *(pD-1);
               }
#else
#if 1
               int i, iVal = iUcnt;
               unsigned long v;
               iVal /= 4;
               iVal++;
               v = *(pD-1);
               v <<= 8;
               v |= *(pD-1);
               v <<= 8;
               v |= *(pD-1);
               v <<= 8;
               v |= *(pD-1);
//               (unsigned long)*((unsigned long*)pD) = v;
               i = 0;
               for (i; i < iVal; i++)
                   (unsigned long)*(((unsigned long*)pD)+i) = v;
               pD += iUcnt;
#else
               memset(pD, *(pD-1), iUcnt);
#endif // 0
#endif // 0
               pSource++;
               goto byteflag_1;
               break;
            }
            case 0x07 :
            {
//               if (iScnt == 0)
//				   break;
			   *pD++= *(pSource+1);
//               byteflag= ~byteflag;
               pSource++;
               iScnt--;
               pSource++;
               goto byteflag_0;
               break;
            }
            case 0x08 :
            {
               L4bits= *(pSource+1) & 0xF0;
               L4bits >>= 4;
               if (L4bits <= 7)
               {
                  L4bits+= 0x07;
               }
               else
               {
                  L4bits+= 234;
               }
               *pD++= *(pD-1)+L4bits;
               pSource++;
               goto byteflag_1;
               break;
            }
            default:
            {
               H4bits= *pSource & 0x0F;
               if (H4bits >6)
               {
                  H4bits+= 240;
               }
               *pD++= *(pD-1)+H4bits;
//               byteflag= ~byteflag;
               pSource++;
               goto byteflag_0;
               break;
            }
         }
      } // while
byteflag_0:
   while ( iScnt > 0 )
   {
      iScnt--;
      {
         switch (*pSource & 0xF0)
         {
            case 0x70 :
            {
               L4bits = *pSource & 0x0F;
               H4bits= *(pSource+1) & 0xF0;
               H4bits >>= 4;
               L4bits <<= 4;
               *pD++= L4bits+H4bits;
//               byteflag =  ~byteflag;
               pSource++;
               goto byteflag_1;
                  break;
            }
            case 0x80 :
            {
               L4bits = *pSource & 0x0F;
               if (L4bits <=7)
               {
                  L4bits+= 0x07;
               }
               else
               {
                  L4bits+= 234;
               }
               *pD++= *(pD-1)+L4bits;
               pSource++;
               goto byteflag_0;
               break;
            }
            case 0x90:
            {
               L4bits = *pSource & 0x0F;
               iUcnt = L4bits+3;
#if 1
               while (iUcnt-->0)
               {
                  *pD++= *(pD-1);
               }
#else
#if 1
               int i, iVal = iUcnt;
               unsigned long v;
               iVal /= 4;
               iVal++;
               v = *(pD-1);
               v <<= 8;
               v |= *(pD-1);
               v <<= 8;
               v |= *(pD-1);
               v <<= 8;
               v |= *(pD-1);
//               (unsigned long)*((unsigned long*)pD) = v;
               i = 0;
               for (i; i < iVal; i++)
                   (unsigned long)*(((unsigned long*)pD)+i) = v;
               pD += iUcnt;
#else
               memset(pD, *(pD-1), iUcnt);
#endif
#endif
               pSource++;
               break;
            }
            default:
            {
               H4bits = *pSource & 0xF0;
               H4bits >>= 4;
               if (H4bits > 6)
               {
                  H4bits+=  240;
               }
               *pD++= *(pD-1)+ H4bits;
               pSource--;
               iScnt++;
//               byteflag = ~byteflag;
               pSource++;
               goto byteflag_1;
               break;
            }
         }
      }
//      pSource++;
   } // while
    
   return  ((int)(pD-pD0));
}
#else
int CRleDeComp::DeZipDeltaRLE ( BYTE *pSource, BYTE *pD, int nByteAnz )
{
   BYTE H4bits, L4bits,*pD0;
   int iScnt,iUcnt, byteflag = 0;
   iUcnt = 0;
   iScnt = nByteAnz;
   pD0   = pD;
   while ( iScnt-- > 0 )
   {
//      iScnt--;
      if (byteflag != 0 )
      {
//         H4bits= *pSource & 0x0F;
//         L4bits= *(pSource+1) & 0xF0;
         switch (*pSource & 0x0F)
         {
            case 0x07 :
            {
               if (iScnt == 0)
				   break;
			   *pD++= *(pSource+1);
               byteflag= ~byteflag;
               pSource++;
               iScnt--;
               break;
            }
            case 0x08 :
            {
               L4bits= *(pSource+1) & 0xF0;
               L4bits >>= 4;
               if (L4bits <= 7)
               {
                  L4bits+= 0x07;
               }
               else
               {
                  L4bits+= 234;
               }
               *pD++= *(pD-1)+L4bits;
               break;
            }
            case 0x09:
            {
               L4bits>>= 4;
               iUcnt= L4bits+3;
               while (iUcnt-->0)
               {
                  *pD++= *(pD-1);
               }
               break;
            }
            default:
            {
               H4bits= *pSource & 0x0F;
               if (H4bits >6)
               {
                  H4bits+= 240;
               }
               *pD++= *(pD-1)+H4bits;
               byteflag= ~byteflag;
               break;
            }
         }
      }
      else
      {
         H4bits = *pSource & 0xF0;
         L4bits = *pSource & 0x0F;
         switch (H4bits)
         {
            case 0x70 :
            {
               H4bits= *(pSource+1) & 0xF0;
               H4bits >>= 4;
               L4bits <<= 4;
               *pD++= L4bits+H4bits;
               byteflag =  ~byteflag;
                  break;
            }
            case 0x80 :
            {
               if (L4bits <=7)
               {
                  L4bits+= 0x07;
               }
               else
               {
                  L4bits+= 234;
               }
               *pD++= *(pD-1)+L4bits;
               break;
            }
            case 0x90:
            {
               iUcnt = L4bits+3;
               while (iUcnt-->0)
               {
                  *pD++= *(pD-1);
               }
               break;
            }
            default:
            {
               H4bits >>= 4;
               if (H4bits > 6)
               {
                  H4bits+=  240;
               }
               *pD++= *(pD-1)+ H4bits;
               pSource--;
               iScnt++;
               byteflag = ~byteflag;
               break;
            }
         }
      }
      pSource++;
   }
    
   return  ((int)(pD-pD0));
}
#endif  // 0
#endif // 1