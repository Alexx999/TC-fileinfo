#if !defined( __tcoptions_H )
#define __tcoptions_H

   struct OPTIONS 
   {
		BOOL autosave;
		BOOL rememberAP;
		BOOL undec;
		BOOL res;
		BOOL debug;
		BYTE MaxDepth;
		BOOL sort;
		BOOL pdata;
		BOOL reloc;
		TCHAR *p_ini;
		TCHAR tlexc[256];
   };

#endif
