#if 0
//
// Generated by Microsoft (R) D3D Shader Disassembler
//
//
//   fxc /nologo /Qstrip_reflect /T ps_4_0 /E PS /Fh
//    Z:/projects/apitrace/tests/apps/d3dcommon/tri_ps_4_0.h
//    Z:/projects/apitrace/tests/apps/d3dcommon/tri.fx
//
//
// Input signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// SV_POSITION              0   xyzw        0      POS   float       
// COLOR                    0   xyzw        1     NONE   float   xyzw
//
//
// Output signature:
//
// Name                 Index   Mask Register SysValue  Format   Used
// -------------------- ----- ------ -------- -------- ------- ------
// SV_Target                0   xyzw        0   TARGET   float   xyzw
//
ps_4_0
dcl_input_ps linear v1.xyzw
dcl_output o0.xyzw
mov o0.xyzw, v1.xyzw
ret 
// Approximately 0 instruction slots used
#endif

const BYTE g_PS[] =
{
     68,  88,  66,  67,  89,  62, 
    128, 137,  52, 137, 121,  63, 
    223, 129, 145, 249,  18, 101, 
     85, 245,   1,   0,   0,   0, 
    244,   0,   0,   0,   3,   0, 
      0,   0,  44,   0,   0,   0, 
    128,   0,   0,   0, 180,   0, 
      0,   0,  73,  83,  71,  78, 
     76,   0,   0,   0,   2,   0, 
      0,   0,   8,   0,   0,   0, 
     56,   0,   0,   0,   0,   0, 
      0,   0,   1,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,  15,   0,   0,   0, 
     68,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   1,   0, 
      0,   0,  15,  15,   0,   0, 
     83,  86,  95,  80,  79,  83, 
     73,  84,  73,  79,  78,   0, 
     67,  79,  76,  79,  82,   0, 
    171, 171,  79,  83,  71,  78, 
     44,   0,   0,   0,   1,   0, 
      0,   0,   8,   0,   0,   0, 
     32,   0,   0,   0,   0,   0, 
      0,   0,   0,   0,   0,   0, 
      3,   0,   0,   0,   0,   0, 
      0,   0,  15,   0,   0,   0, 
     83,  86,  95,  84,  97, 114, 
    103, 101, 116,   0, 171, 171, 
     83,  72,  68,  82,  56,   0, 
      0,   0,  64,   0,   0,   0, 
     14,   0,   0,   0,  98,  16, 
      0,   3, 242,  16,  16,   0, 
      1,   0,   0,   0, 101,   0, 
      0,   3, 242,  32,  16,   0, 
      0,   0,   0,   0,  54,   0, 
      0,   5, 242,  32,  16,   0, 
      0,   0,   0,   0,  70,  30, 
     16,   0,   1,   0,   0,   0, 
     62,   0,   0,   1
};
