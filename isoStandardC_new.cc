/*============================================================================
* Program to parse ISO/IEC STANDARD 19794-7 format Hex file of Signature 
* Template.
*
* Author: Ankit Singh (ankit.singh@sit.fraunhofer.de OR 
*                      ankitsingh.05@gmail.com )
* 
* @Description:
* This modules takes input ISO standard Signature template format for the
* processing data and follows all recommendations made in ISO standard 19794-7 
* document. This module finds the scaling value for 'x', 'y' position and 
* pressure 'p' (force).
* 
* @Output: 
*  - Use scaling Value of x, y for converting it to its original value
*  - Writes the output to a file seg_x, seg_y and seg_f
*
* @TODO: write a separate file for segment information
* 
* @Status: Partially Complete (Further Development accourding to the requirement)
* 
* @COMPILATION: 
*  For Linux/Unix/Windows based System:
*        > g++ -g isoStandardC.cpp -o isoParse
*  For Windows OS:
*        Install g++ compiler using instructions given in the link below
*             http://www.claremontmckenna.edu/pages/faculty/alee/g++/g++.html
*		 > g++ -g isoStandardC.cpp -o iso -lm
*
* @USAGE:
*   ./isoParse samples/new2.sign
*=============================================================================*/

#include "isoStandardC_new.h"

int main(int argc, char *argv[]){
	vector<int> buffer;
	const int patternSum = 6;
	bool patFound = false;
	char prevChar;
	char byte;	
	unsigned int byte1, byte2, maskedByte;
	int xscale, yscale, tscale, length;
	int countPat = 0,efVal[2];

	// Command line check
	if ( argc != 2 ) // argc should be 2 for correct execution
		cout<<"\n USAGE: "<< argv[0] <<" <ISO Hex File>\n" << endl;
	else {
		cout << "\n Input Filename: " << argv[1] << endl;
		ifstream OpenFile((char *) argv[1], ios::in);

		if (OpenFile.is_open()){		
			while(!OpenFile.eof()){	

				while(OpenFile.get(byte)){
					/* For Finding PATTERN 'SDI' for conforming ISO Standard Signature format */
					if(byte == 'S'){
						if(prevChar != 'S') // avoiding consecutive same char 's' at beginning
							countPat = countPat + 1;
					} else if (byte == 'D' )
						countPat = countPat + 2;
					else if(byte == 'I')
						countPat = countPat + 3;
					else
						countPat = 0;

					prevChar = byte; // this variable will check non repitition of the character

					// Authenticate the template & allow for further processing of Data
					if(countPat == patternSum){
						cout << "\n\t *** ISO/IEC STANDARD 19794-7 Signature Template Found *** \n\n\t Processing the Template File......  \n" << endl;
						patFound = true;
						countPat = 0; //reinitilize for not entering this if condition
						break;
					}
				}// END GET

				/* WARNING message: ISO standard not found */
				if(patFound != true){
					cout << "\n\n\t* Oooops!!! NOT a ISO/IEC STANDARD 19794-7 Signature Template or Bad Data *\n" << endl;
					cout << "\n Cannot Process Data Further!!! \n\n ** PLEASE GIVE ISO/IEC STANDARD 19794-7 Signature Template For Processing **" << endl;
					cout<<"\n USAGE: "<< argv[0] <<" <ISO Hex File>\n" << endl;
				} 
				/* Now the Processing of the DATA begins from here */
				else { 
					while(OpenFile.get(byte)){
						// XSCALE Value Extraction
						maskedByte = (unsigned int) byte;
						if(maskedByte == 0x80) {
							efVal[0] = 0;  efVal[1] = 0; 

							cout << "\n ** X Scale **" << endl;
							OpenFile.get(byte); byte1 = (unsigned int) byte; 
							OpenFile.get(byte); byte2 = (unsigned int) byte; 
							scaleExtract(byte1, byte2, efVal); // Extraction of exponent E & mantissa F
							xscale = (int) scalingOp(efVal); // Calculation of Scaling Value from E & F

							/* YSCALE Value Extraction */
							cout << "\n ** Y Scale **" << endl;
							OpenFile.get(byte); // skip 0x80 for YSCALE
							OpenFile.get(byte); byte1 = (unsigned int) byte; 
							OpenFile.get(byte); byte2 = (unsigned int) byte; 
							scaleExtract(byte1, byte2, efVal); // Extraction of exponent E & mantissa F
							yscale = (int) scalingOp(efVal); // Calculation of Scaling Value from E & F
							OpenFile.get(byte); // next one byte

						} // End if 0x80

						/* DT scaling operation Here */
						if(byte == 0x84){
							efVal[0] = 0;  efVal[1] = 0; 
							cout << "\n ** DT Scale **" << endl;
							OpenFile.get(byte); byte1 = (unsigned int) byte;
							OpenFile.get(byte); byte2 = (unsigned int) byte; 
							scaleExtract(byte1, byte2, efVal); // Extraction of exponent E & mantissa F
							tscale = (int) scalingOp(efVal); // Calculation of Scaling Value from E & F
							OpenFile.get(byte); // skip one byte
						}

						/* FORCE MINIMUM & MAXIMUM value included AND X,Y,F */
						if(byte == 0x60){
							int minForce, maxForce;
							OpenFile.get(byte); byte1 = (unsigned int) byte;
							OpenFile.get(byte); byte2 = (unsigned int) byte; 
							minForce =  bit16toDec(byte1,byte2);
							cout << "\n ** Minimum Force: " << minForce;
							OpenFile.get(byte); byte1 = byte;
							OpenFile.get(byte); byte2 = byte; 
							maxForce =  bit16toDec(byte1,byte2);
							cout << "; Maximum Force: " << maxForce << " **" << endl;
							OpenFile.get(byte); OpenFile.get(byte); // RESERVED (SKIP) 
							OpenFile.get(byte); // NO EXTENDED DATA (SKIP)

							/* Length of the record */
							OpenFile.get(byte); byte1 = byte;
							OpenFile.get(byte); byte2 = byte; 
							length =  bit16toDec(byte1,byte2);
							cout << "\n ** Sample Length: " << length << " **" << endl;

							/*FUNCTION FOR EXTRACTING X, Y, Z VALUE*/
							parseXyF(OpenFile,xscale, yscale, tscale);
						}
					} // END while get (patFound)
				} // END else PatFound
			}// END eof
		} // END if
	} // END ELSE ARGC
} // END MAIN

int bit16toDec(unsigned int byte1, unsigned int byte2){
	//printf("byte1 = %x\n", byte1);
	//printf("byte2 = %x\n", byte2);

	unsigned int firstDec = (byte1 & 0xff) << 8;
	//cout << "\n FirstDec: " << firstDec << endl;
	unsigned int secondDec = byte2;
	//cout << "\n SecondDec: " << secondDec << endl;

	int result = firstDec + secondDec;
	//cout << "\n ****result: " << result << endl << endl;
	return result;
}

void scaleExtract(unsigned int byte1, unsigned int byte2, int efVal[]){
	//printf("byte1 = %x\n", byte1);
	//printf("byte2 = %x\n", byte2);
	//unsigned int byte1 = 0xeb;
	//unsigned int byte2 = 0x89;
	unsigned int exp = (byte1 & 0xf8) >> 3; // 11111000
	//printf("exp:%x\n",exp);
	unsigned long mantissa = ((byte1 & 0x03) << 8) + byte2; //00000111
	//printf("mantissa:%lx\n", mantissa);

	efVal[0] = exp;  efVal[1] = mantissa; 
	printf("exp.mantissa decimal = %d.%d\n", (int)exp, (int)mantissa);
}

// calculation of scaling operation
float scalingOp(int efVal[]){
	int E, F;
	float scaleV=0; 
	float pow1 = 0, pow2 = 0;
	E = efVal[0];
	F = efVal[1];
	pow1 = power(2,11); 
	pow2 = power(2,(E-16));
	//Scaling formula. Please refer ISO standard document for details
	scaleV = (1+F/pow1) * pow2 ;
	cout << "Scale Value: " << scaleV << endl;
	return scaleV;
}

/* Function for 'num^pow' i.e pow(num,pow) */
int power(int num, int pow){
	long int p;                                                        
	for (int n = 0; n <= pow ; n++)    /* LOOP BEGINS */
	{															
		if (n == 0) p = 1;                                               
		else p = p * num;										       							       
	}  
	//cout << "\n Power: " << p << endl;	 
	return p;
}

/*
Extracting Value of X, Y and F. 
- (OPTIONAL) Use scaling Value of x, y for converting it to its original value
- Writes the output to a file seg_x.txt, seg_y.txt, seg_f.txt and seg_t.txt
*/
void parseXyF(ifstream &OpenFile, int xscale, int yscale, int tscale){
	//FILE* seg_x, seg_y, seg_f, set_t;
	int seq = 0;
	bool firstSampleTime = true;
	float sampleTime=0;
	unsigned char byte;	
	unsigned int byte1, byte2;
	ofstream segx("seg_x.txt"); ofstream segy("seg_y.txt"); 
	ofstream segf("seg_f.txt"); ofstream segt("seg_t.txt");

	/*Time Calculation (unit = mm [Milli Second])*/
	sampleTime = (float)(1000/tscale); // in mm
	cout << "\n DT Sample Time: " << sampleTime << " mm"<<endl;

		if (segx.is_open() && segy.is_open() && segf.is_open() &&  segt.is_open()){
			while(!OpenFile.eof()){
				if(seq == 0){ // for X co-ordinate
					OpenFile.get(byte); byte1 = byte;
					OpenFile.get(byte); byte2 = byte; 
					segx << bit16toDec(byte1,byte2) << "\n";
					//cout << "X: " << bit16toDec(byte1,byte2) << endl;
					/* Time writing into the File */
					if(firstSampleTime != false) {segt << 0 << "\n"; firstSampleTime = false;}
					else segt << sampleTime << "\n";
					seq = 1;
				}

				if(seq == 1){ // for Y co-ordinate
					OpenFile.get(byte); byte1 = byte;
					OpenFile.get(byte); byte2 = byte; 
					segy << bit16toDec(byte1,byte2) << "\n";
					//cout << "Y: " << bit16toDec(byte1,byte2) << endl;
					seq = 2;
				}

				if(seq == 2){ // for Y co-ordinate
					OpenFile.get(byte); byte1 = byte;
					OpenFile.get(byte); byte2 = byte; 
					segf << bit16toDec(byte1,byte2) << "\n";
					//cout << "F: " << bit16toDec(byte1,byte2) << endl;
					seq = 0;
				}
			} // END eof
		} // END if open()
		segx.close(); segy.close(); segf.close(); segt.close(); // FILE CLOSE
		cout << "\n\n\t*** The OUTPUT is written to FILES ----->>> seg_x.txt, seg_y.txt, seg_f.txt and seg_t.txt" << endl << endl;
}