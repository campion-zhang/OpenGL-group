VERT

DCL IN[0]
DCL IN[1]
DCL OUT[0], POSITION
DCL OUT[1], COLOR
DCL CONST[0][1]
DCL CONST[0][3]
DCL TEMP[0..1]

MOV OUT[0], IN[0]
ADD TEMP[0], IN[1], CONST[0][1]
RCP TEMP[1], CONST[0][3].xxxx
MUL OUT[1], TEMP[0], TEMP[1]

END