;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; v 0.1
;;
;; Deasemblacja zawartości BIOS BOOT komputera Elwro 800 Junior.
;;
;; Nie jest to pełna deasemblacja, nie można jej skompilować bo zawiera wiele błędów składniowych.
;; Nie wszystkie fragmenty zostały rozpracowane. Komentarze wymagają jeszcze poprawy w wielu 
;; miejscach.
;;
;; 
;; Krzysztof Komarnicki 25.10.2006
;;
;;


     ORG 0x0000
L0000:			      ; RST 00
     LD        A,0x40       
     OUT       0xf7,A         ; załączenie ROM BOOT, wyłączenie NMI
     LD        HL,0x3db0      ; wyzerowanie obszaru danych RAM 0x3DB0...0x3FFF
L0007:
     LD        (HL   ),0x00   ;    
     INC       HL        
     LD        A,H         
     CP        0x40         
     JR        NZ,L0007       ; for(HL = 0x3db0; HL < 0x4000; HL++) *HL = 0;
     JP        L1794          ; dalszy ciąg procedury pod adresem L1794 (test pamięci)

    ORG 0x0019
L0019:
    JP        0x0100        

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Funkcja powrotu do ZX BASIC BIOS
;; tu kończą wywołania z ZX BASIC BIOS
;;
;;
;;

     ORG 0x0028
L0028:                    ; RST #28
     LD        A,0x00       
     EI                    
     OUT       0xf7,A     ; powrót do ROM ZX BASIC
;0x002D adres w ROM BASIC JP 0x39B2
; lub, gdyby przełączenie nie nastąpiło  jp 150 ??
     JP        0x0150        

L0030:			  ; RST #30 
    RST        0x38

    ORG 0x0038
L0038:
    RET                   ; RST #38 lub INT w IM1

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Dalsza część obsługi RST #00 - spod adresu #1794
;; po teście RAM i ROM, oraz po komunikatach ekranowych i wyświetleniu logo
;;
;; Skopiowanie obszaru UDG (semigrafiki) z ROM BAS1 do RAM  
;;
     ORG 0x0040
L0040:
     LD        HL,(0x5c7b)    ; HL = *(UDG) - adres UDG
     LD        BC,0x0088      ;
     ADD       HL,BC          ; 
     LD        DE,0x3c80      ; 
     EX        DE,HL          ; DE = *(UDG)+88; HL = 0x3c80
     LD        BC,0x0020      ; 32 bajty
     LD        A,0xc0         ; załączenie górnej połówki ZX BASIC ROM
     OUT       0xf7,A         ; 0..1FFF - BOOT ROM, 2000..3FFF - BAS1 ROM
     LDIR                     ; skopiowanie skopiowanie obszaru UDG z BAS1 do RAM
     JP        0x0080         ; dalszy ciąg pod adresem #80 i dalej pod #141

T0057:
    DB 0xff,0xff,0xff,0xff,0xff

  5c LD        A,0x00       
  5e EI                    
  5f OUT       0xf7,A       
  61 JP        0x01af        
  64 RST       56         
  65 RST       56         

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Obsługa NMI
;;
;;
L0066:
     RET

L0067:
  67 NOP                   
  68 EI                    
  69 OUT       0xf7,A       
  6b JP        0x0162        

  6e RST       56         
  6f AND       A           
  70 OUT       0xf7,A       
  72 JP        0x190c        
  75 LD        A,0x00       
  77 JP        0x0067        

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Ciąg dalszy procedury startowej spod adresu #40
;; 
;;
    ORG 0x0080
L0080:
    JP        0x0141        

L0083:
    JP        0x04eb        

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Wyświetlenie napisu:
;; 
;; "wersja dyskowa"
;;
;;
    ORG 0x0090
L0090:
  90 LD        HL,0x1755     
  93 LD        DE,0x4137     
  96 JR        0x009e         

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Wyświetlenie napisu:
;; 
;; "wersja bezdyskowa"
;;
;;
L0098:
     LD        HL,L1732      ; 
     LD        DE,0x4136     ;
L009E:
     CALL      L00A7	     ; L00A7(0x1732, 0x4136) - "wersja [bez]"
     LD        HL,L1771      ;
     LD        DE,0x413b     ; L00A7(0x1771, 0x413b) - "dyskowa"			     

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;  Skopiowanie pięciu 7 liniowych znaków na ekran
;;
;;  Par wejściowe:
;;   HL - skąd kopiować
;;   DE - w jakie miejsce
;;
L00A7:
     LD        C,0x05       ; for(C=5; C ; C--){
L00A9:
     LD        B,0x07       ;  	B = 7
     PUSH      DE           ;  	stk = DE
L00AC:			    ;  	do{
     LD        A,(HL  )     ;  
     LD        (DE),A       ; 		*DE = *HL
     INC       HL           ;   	HL++
     INC       D            ;   	DE += 256;
     DJNZ      L00AC        ;  	} while( --B )
     POP       DE           ;  	DE = stk
     INC       DE           ;  	DE++      - następny znak
     DEC       C            ;  
     JR        NZ,L00A9     ; }
     RET                    ;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Funkcja:
;;   Skopiowanie dziesięciu, 7-liniowych znaków na ekran
;; HL- skąd kopiować
;; DE- w jakie miejsce
;;
L00B8:
     LD        C,0x0a ; wyświetlamy 10 znaków
     JR        0x00a9 ; skok do procedury wyświetlania znaków
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
    ORG 0x00c0
T00C0:
    DB "(C) T.Koszlajda IAPP 1986"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;
;;
;;
;;
;;
    ORG 0x0100
L0100:
 100 DI                    
 101 LD        SP,0x8000     
 104 LD        A,0x40       
 106 OUT       0xf7,A       
 108 LD        A,0x09       
 10a OUT       0xf1,A       
 10c OR        A           
 10d PUSH      AF          
 10d POP       IY        
 10f LD        BC,0xdfb0     
 112 CALL      0x05ae        
 116 LD        BC,0x0133     
 119 LD        DE,0xdfb0     
 11c CALL      0x0083        
 11f OR        A           
 120 JP        NZ,0x0116     
 123 LD        BC,0x013a     
 126 LD        DE,0xdfb0     
 129 CALL      0x0083        
 12c OR        A           
 12d JP        NZ,0x0123     
 130 JP        0xce00        

 133 NOP                   
 134 INC       B           
 135 ADD       HL,BC     
 136 NOP                   
 137 LD        BC,0xb800     
 13a NOP                   
 13b INC       B           
 13c ADD       HL,BC     
 13d LD        BC,0x0001     
 140 DB 0xca

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Ciąg dalszy procedury startowej RST #00, po teście pamięci
;; i skopiowaniu obszaru UDG do RAM
;;
;;
L0141:
     LD	       A,0x40		;
     OUT       0xf7,A       	; wyłączenie ROM BAS1, załączonego podczas kopiowania UDG
     LD        HL,0x3209     
     LD        (HL   ),0x00  
     LD        BC,0x2000     
     JP        0x05d9           ; (0x3209) = 0; BC=0x2000; ciąg dalszy pod adresem #5d9

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Punkt wejścia z BIOS-u ZX BASIC do BIOS-u BOOT. 
;;
;; 
;;

L014F:
     PUSH      IY            ; zachowanie adresu zmiennych systemoiwych interpretera BASIC
     CALL      0x13f5        ; funkcja obsługi rozkazu DIR
     POP       IY            ; odtworzenie adresu zmiennych BASIC
     LD        A,(0x3208)    
     OR        A           
     JP        Z,0x0028      
     SCF                   
     JP        0x0028        ; powrót do ZX BASIC




 161 PUSH      IY        
 163 PUSH      BC          
 164 PUSH      DE          
 165 PUSH      IY        
 166 LD        C,0x00       
 168 PUSH      BC          
 169 
 16a PUSH      IY        
 16b POP       BC          
 16c LD        DE,0x0011     
 170 JP        NC,0x01cf     
 173 CALL      0x103c        
 176 POP       HL        
 177 POP       DE          
 178 POP       BC          
 178 POP       IY        
 17a LD        A,(0x3208)    
 17d OR        A           
 17e JP        Z,0x018c      
 181 CP        0x02         
 183 SCF                   
 184 JP        Z,0x0075      
 187 CCF                   
 188 JP        0x0075        
 18b ILLEGAL              
 18d PUSH      HL        
 18e PUSH      BC          
 18f PUSH      DE          
 190 PUSH      HL        
 191 LD        C,0xff       
 193 PUSH      BC          
 194 LD        C,L         
 195 LD        B,H         
 196 CALL      0x103c        
 199 POP       HL        
 19a POP       DE          
 19b POP       BC          
 19b POP       IY        
 19d LD        A,(0x3208)    
 1a0 OR        A           
 1a2 JP        Z,0x0075      
 1a5 CP        0x02         
 1a7 SCF                   
 1a8 JP        Z,0x0075      
 1ab CCF                   
 1ac JP        0x0075        
 1ae PUSH      IY        
 1b0 PUSH      IY        
 1b1 PUSH      DE          
 1b2 PUSH      BC          
 1b3 
 1b4 PUSH      IY        
 1b5 POP       BC          
 1b6 CALL      0x12ad        
 1b9 POP       DE          
 1bb POP       HL        
 1bb POP       IY        
 1bd LD        A,(0x3208)    
 1c0 OR        A           
 1c1 JP        Z,0x005c      
 1c4 CP        0x02         
 1c6 SCF                   
 1c7 JP        Z,0x005c      
 1ca CCF                   
 1cb JP        0x005c        
 1ce CALL      0x14e1        
 1d1 POP       IY        
 1d2 POP       DE          
 1d3 POP       BC          
 1d4 ILLEGAL              
 1d6 POP       HL        
 1d7 LD        A,(0x3208)    
 1da OR        A           
 1db JP        Z,0x0075      
 1de CP        0x02         
 1e0 SCF                   
 1e1 JP        Z,0x0075      
 1e4 CCF                   
 1e5 JP        0x0075        
 1e8 POP       HL        
 1e9 EX (SP),HL            
 1ea LDIR                  
 1ec RET                   

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Załączenie silnika napędu FDD
;;
;; C - numer napędu, 0 - A, 1- B
;;
L01ED:
     LD        A,C         
     INC       A       ; aby się ustaw bitów zgadzało     
     OR        0x08    ; reset = 0     
     OUT       0xf1,A  ; ustaw port
     OR        A       ; CY = 0
     PUSH      AF          
     POP       IY      ; IY = AF ????
     RET                   

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Wyłączenie silników napędu FDD
;;
;; 
;;
L01F8:
     PUSH      IY        
     POP       AF          
     AND       0xfc         
     OUT       0xf1,A       
     RET                   

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Pobranie/wysłanie pakietu danych do operacji READ, WRITE i FORMAT
;;
;; Funkcja odczytuje (zapisuje) 512 bajtów do FDC
;; DE - adres bufora zapisu/odczytu
;; IY - IYH zawiera maskę dla rejestru FDC 0xF1
;;
;;

L0200:
     EX        DE,HL       
     LD        B,0x00       ; 256 bajtów do odczytania/zapisu x 2
     LD        A,C          ; A = C   
     CP        0x06         
     LD        C,0xef       ; C = 0xef
     JP        Z,0x022a     ; if( A == 6) goto L022A /* jeśli komenda READ DATA */
L020B:
     IN        A,0xee       ; pobierz msr FDC
     BIT       7,A         
     JR        Z,0x020b     ; czekaj na bit gotowości danych RQM
     BIT       5,A          ; czy ustawione DMA ?
     JR        Z,0x0253     ; jeśli tak to return;
     OUTI                   ; (C) <- (HL)
     JR        NZ,0x020b    ; zapisz następną daną
L0219:
     IN        A,0xee       
     BIT       7,A         
     JR        Z,0x0219     ; czekaj na gotowość danej  
     BIT       5,A         
     JR        Z,0x0253     ; wyjdź
     OUTI                  
     JR        NZ,0x0219    ; zapisz następną daną
     JP        0x0246        
L022A:
     IN        A,0xee       
     BIT       7,A         
     JR        Z,0x022a       ; czekaj na gotowość danej (bit RQM)
     BIT       5,A            ; BIT NO_DMA
     JR        Z,0x0253       ; jak DMA to return
     INI                      ; (HL) <- IN(C); B--; HL++
     JR        NZ,0x022a      ; jeśli B !=0 goto L022A

L0238:
     IN        A,0xee       
     BIT       7,A         
     JR        Z,0x0238       ; czekaj na gotowość danej (bit RQM)
     BIT       5,A         
     JR        Z,0x0253       ; no to return
     INI                   
     JR        NZ,0x0238      ; jeśli B != 0 goto L0238
L0246:
     PUSH      IY        
     POP       AF             ; AF = IY
; Wysłanie TC do FDC
     AND       0x03           ; A &= 0x03; /* run0 =0, run1 =0*/
     OR        0x0c           ; A |= 0x0c; 0000 1100 /* TC=1, RESET=0 */
     OUT       0xf1,A         ; outb(0xf1, (IYH & 0x03) | 0x0c);
     AND       0xfb           ; A &= 0xfb; /* 1111 1011 TC=0, RESET=0 */
     OUT       0xf1,A	      ; outb(0xf1, IYH & 0xfb);
L0253: 	
     RET

T0254:
 DB 0xa0	; maksymalny numer sektora na ścieżce

T0255:
 DB 0x00	; minimalny numer ścieżki
                    
 256 LD        (BC),A      
 257 LD        (BC),A      

T0258:
     DB 0x02 ; ilość bajtów na sektor

T0259:
     DB 0x40 ; pole MT MFM
T025A: 
     DB 0x1a ; pole GPL


 25b INC       H           
 25c NOP                   

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 
;; Czyzby zamierzeniem autora było umieszczenie tego jako zmienne w RAM, ale "w praktyce"
;; okazało się że nie warto i umieszczono je w ROM ? Inaczej po co byłaby procedura
;; obliczania parametrów, wg stałych :|
;;

; wartość pola SRT w instrukcji SPECIFY kontrolera FDC
C025D:
    DB 0x02
; wartość pola HLT w instrukcji SPECIFY kontrolera FDC
C025E:
    DB 0x80
; wartość pola HUT w instrukcji SPECIFY kontrolera FDC
C025F:
    DB 0x00
C0260:
    DB 0x00

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Wywoływane jako pierwsza funkcja w 5d9
;;
;; Funkcja wysyła komendę SPYCIFY do kontrolera FDC
;;
;; DE - 0x2000 - adres bazowy bufora rozkazu dla kontrolera FDC
;;
L0261:
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; Poniższy kod OBLICZA STAŁE !!!:                     ;;
;; *(DE + 09d) = 0x03 = instr_code                     ;;
;; *(DE + 10d) = 0xe0 = (SRT << 4) | ((HUT & 0xf8)>>4) ;;
;; *(DE + 11d) = 0x81 = (HLT & 0xfe) | (ND & 1)        ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
     LD        HL,0x0009     
     ADD       HL,DE     	; HL = DE +9
     LD        (HL   ),0x03  	; *(DE + 9) = 0x03	- kod instrukcji SPECIFY dla FDC
     LD        A,(0x025d)       ; A = (0x25d) ->  A=2   - wartość pola SRT
     CPL                   	; A = fd
     INC       A           	; A = -2	
     ADD       A,A         	; A = -4
     ADD       A,A         	; A = -8
     ADD       A,A         	; A = -16
     ADD       A,A         	; A = (-(0x25d)) << 4 = -32 = -0xe0
     PUSH      AF          	; push A
     LD        A,(0x025f)    	; A = 0			- wartość pola HUT
     AND       0xf8         	; hmm błąd ??, wg mojej dok. pole HUT ma 4 bity, więc skąd f8 ??
     RRA                   	; 
     RRA                   	; 
     RRA                   	; 
     RRA                   	; A = 0 A = A >> 4 = 0
     POP       BC          	; C = B = -32 (0xe0)
     LD        C,B         
     OR        C           	; A = (SRT << 4) | (HUT >> 4)= 0x0e  - złożenie pola SRT i HUT razem
     INC       HL        	; HL = DE + 10
     LD        (HL  ),A 	; *(DE+10) = -32	- zapis danej do bufora
     LD        A,(0x025e)       ; A = 0x80		- wartość pola HLT
     AND       0xfe             ; A = 0x80		- maskowanie bitowe dla tego pola
     PUSH      AF          
     LD        A,0x01       	; A=1                   - wartość pola ND (1 bit) - BEZ DMA
     AND       0x01         	; 			- maskowanie bitowe dla tego pola
     POP       BC          	; B = 0x80
     LD        C,B         	; C = B = 0x80
     OR        C           	; A = HLT | ND		- złożenie pól bitowych
     INC       HL        	; HL = DE +11
     LD        (HL  ),A 	; *(DE+11) = 0x81	- zapis danej do bufora
     LD        BC,0x0007     
     EX        DE,HL            ; 
     ADD       HL,BC            ; 
     LD        C,L         	; BC = HL = DE + 07, DE = DE + 0b
     LD        B,H         
     CALL      L099A            ; wysłanie rozkazu do FDC
     RET                   

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Funkcja:
;;   Wykonanie operacji SEEK lub RECALIBRATE
;;
;; Wejście:
;; 	B  - nr napędu
;; 	C  - NCN | HDS (numer cyllindra | numer głowicy); jeśli 0 to operacja RECALIBRATE
;; 	DE - adres bufora instrukdcji
;; 	
;;  DE+9,10,11 - bufor instrukcji (de + 2,3,4 po stronie 99a)
;;  DE+1a      - ? (de+13 po stronie 99a )
;;  DE+2f      - wynik operacji dla disk0
;;  DE+30      - wynik operacji dla disk1
;;
L0299:
     LD        A,C         	
     CP        0x00         	; jaki rodzaj instrukcji ?
     JP        NZ,L02AA     	; jeśli SEEK skocz pod 2AA
; dla C=0
     LD        HL,0x0009     
     ADD       HL,DE     
     LD        (HL   ),0x07  	; (DE+9)  = 7	- kod instrukcji RECALIBRATE FDC
     INC       HL        
     LD        (HL  ),B 	; (DE+10) = B	- nr napędu
     JP        L02B8            ; goto 0x02B8
L02AA:
     LD        HL,0x0009     	
     ADD       HL,DE     
     LD        (HL   ),0x0f  	; (DE+9) = 0x0f - kod instrukcji SEEK FDC
     INC       HL        	
     LD        A,C         	; A = C		- BIT HDS
     AND       0x01             ; A &= 1
     ADD       A,A         	; A += A *2
     ADD       A,A         	; A += A *4
     OR        B           	; A |= B	- nr napędu
     LD        (HL  ),A 	; (DE+10) = B | (A << 3)
; kod instrukcji ustalony
L02B8:
     INC       HL        
     OR        A         	; CY = 0  
     LD        A,C         	; 
     RRA                   	; 
     LD        (HL  ),A 	; (DE + 11) = C >> 1 - NCN (nowy numer cylindra)
; wysłanie kodu instrukcji do FDC
     PUSH      BC          	; zachowaj parametry wejściowe
     PUSH      DE          	; zachowaj parametry wejściowe
     EX        DE,HL       
     LD        BC,0x0007     	
     ADD       HL,BC     
     LD        C,L         
     LD        B,H         	; BC = DE + 7 -> adres bufora rozkazu
     CALL      0x099a        	; wyślij komendę do FDC
     POP       DE          	; odtwórz parametry wejściowe
     POP       BC          	; odtwórz parametry wejściowe
; uaktualnienie zmiennych systemowych ?? 
     LD        HL,0x001a     
     ADD       HL,DE     
     LD        A,(HL  ) 	; 
     CP        0x00         	
     JP        NZ,L02e3     	; if (DE + 0x1a) !=0 goto 2e3
     LD        L,B         	
     LD        H,0x00       	 
     PUSH      BC          
     LD        BC,0x002f     
     ADD       HL,BC     	
     ADD       HL,DE     	; HL = DE + 0x2f + B  (B-numer napędu)
     POP       BC          
     LD        (HL  ),C 	; (HL) = C ( C - NCN | HDS )
     JP        L02ED		; return
L02E3:
     LD        L,B         	
     LD        H,0x00       	
     LD        BC,0x002f     
     ADD       HL,BC     
     ADD       HL,DE     	; HL = DE + 0x2f + B  (B-numer napędu)
     LD        (HL   ),0xff  	; (HL) = 0xff
L02ED:
     RET                   	; return

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Funkcja obsługi błędu operacji FDC: READ/WRITE DATA
;;
;; Wywoływana, gdy kod błędu po operacji FDC jest równy 2. Ustawia odpowiedni kod błędu.
;; 
;; DE - wskaźnik na obszar zmiennych
;;
;; dane wejściowe:
;; (DE+0x12) - kod błędu po operacji FDC: READ/WRITE DATA -> ferr
;; (DE+0x13)
;; (DE+0x14)
;;
;; Zmienne wykorzystywane wewnątrz:
;; (DE+0x35)
;; (DE+0x36)
;; (DE+0x37)
;;
;; dane wyjściowe:
;; A - kod błędu
;;
;; char fdc_error( char *de)
;; {
;; 	uchar err_0, err_1, err2, a;
;;
;;      a = *(de + 0x12);
;;	if( a & 0xd0 == 0x10 ) return 0x10;
;;
;;	err_0 = *(de + 0x35) = (a & 0xfc) >> 3;
;;	st0 = *(de + 0x36) = *(de+0x13);  /* ST0 */
;;	st1 = *(de + 0x37) = *(de+0x14);  /* ST1 */
;;
;;	if( err_0 & 1 )	 return 0x80;
;;	if( st0 & 1 )  	 return st1 & 1 ? 0x0f: 0x0e;   /* błąd nap. 0, 0x0f jeśli brak znacznika  */
;;	if( st0 & 2 )  	 return 0x20; 		        /* błąd nap. 1 */
;;	if( err_0 & 2 )	 return err_0 & 4 ? 0x04 : 0x40;
;;	if( st0 & 0x10 ) return 0x10;			/* błąd FDD, brak track 0 */
;;	if( st0 & 0x80 ) return 0x08;			/* błędny rozkaz FDC ?? */
;;	if( st1 & 1 || st1 & 8 ) return 0x04; 		/* brak znacznika lub overrun */
;;	if( st0 & 0x20 && st1 & 0x20) return 0x02;	/* data CRC error lub */
;;	if( st1 & 0x40 ) return 0x01;
;;	if( ~st1 & st0 & 0x20 ) return 0x0a;
;;	if( st0 & 4 ) 	return 0x08;
;;	return 0xae;
;; }
;;
;; Zwracane kody błędów:
;;	
;;	0x01 -
;;	0x02 -
;;	0x04 -
;;	0x08 -
;;	0x0a -
;;	0x0e -
;;	0x0f -
;;	0x10 -
;;	0x20 -
;;	0x40 -
;;	0x80 -
;;	0xae -
;;

L02EE:
     LD        HL,0x0012     
     ADD       HL,DE     
     LD        A,(HL  ) 	; a = ferr
     AND       0xd0         
     CP        0x10         
     JP        NZ,0x02fd        ; if(a & 0xd0 != 0x10) goto L02FD
     LD        A,0x10           ;
     RET                   	; return 0x10;

L02FD:
     LD        A,(HL  ) 	; a = ferr
     AND       0xfc         	; a &= 0xfc; -> 0 xxxxxx00
     RRA                   	; a >> 1
     RRA                   	; a >> 1
     RRA                   	; a >> 1, -> 00xxxxxx x ; CY = bit2 A
     EX        DE,HL       
     LD        BC,0x0035     
     ADD       HL,BC     
     EX        DE,HL       	; DE = DE+35, HL = DE+12
     LD        (DE),A      	; (DE+35) = (ferr & 0xfc) >> 3;
     INC       HL        
     LD        A,(HL  ) 	; 
     INC       DE          
     LD        (DE),A      	; (DE+36) = (DE+13)
     INC       HL        
     LD        A,(HL  ) 	; 
     INC       DE          
     LD        (DE),A      	; (DE+37) = (DE+14)
     DEC       DE          
     DEC       DE          
     LD        A,(DE)      	; A=(DE+35)
     RRA                   
     JP        NC,0x031c     	; if( (DE+35) & 1 ) return 0x80;
     LD        A,0x80       
     RET                   

L031C:
     INC       DE          
     LD        A,(DE)      	; A = (DE+36)
     RRA                   
     JP        NC,0x032e     	; if( (DE+36) & 1){
     INC       DE          
     LD        A,(DE)      	; 
     RRA                   
     JP        NC,0x032b     	; if( (DE+37) & 1) return 0x0f;
     LD        A,0x0f       	;   else return 0x0e;
     RET                   	;}

L032B:
     LD        A,0x0e       
     RET                   

L032E:
     LD        A,(DE)      	; A = (DE+36)
     OR        A           	; CY=0
     RRA                   
     RRA                   
     JP        NC,0x0338     	; if( (DE+36) & 2) return 0x20
     LD        A,0x20       
     RET                   

L0338:
     DEC       DE          
     LD        A,(DE)      	; A = (DE+35)
     OR        A           
     RRA                   
     RRA                   
     JP        NC,0x034f     	; if( (DE+35) & 2 ){
     LD        A,(DE)      
     AND       0xfe         
     RRA                   
     RRA                   
     RRA                   
     JP        NC,0x034c     	; if( (DE+35) & 4) return 0x04; else return 0x40;
     LD        A,0x04       	
     RET                   	;}

L034C:
     LD        A,0x40       
     RET                   

L034F:
     INC       DE          
     LD        A,(DE)          ; A=(DE+36)
     AND       0xf8         
     RRA                   
     RRA                   
     RRA                   
     RRA                   
     RRA                   
     JP        NC,0x035e     	; if( (DE+36) & 0x10) return 0x10;
     LD        A,0x10       
     RET                   

L035E:
     LD        A,(DE)      	; A=(DE+36)
     RLCA                  	; cy<-n xxxxxxxn <-cy
     RRA                   	; 
     JP        NC,0x0367     	; if( (DE+36) & 0x80) return 0x08;
     LD        A,0x08       
     RET                   

L0367:
     INC       DE          
     LD        A,(DE)      	; A= (DE+37)
     OR        A           
     RRA                   
     PUSH      AF          
     LD        A,(DE)      
     AND       0xf8         
     RRA                   
     RRA                   
     RRA                   
     RRA                   
     POP       BC          
     LD        C,B         
     OR        C           
     RRA                   
     JP        NC,0x037d     ; if(  (DE+37) & 1 || (DE+37) & 8) return 0x04;
     LD        A,0x04       
     RET                   

L037D:
     LD        A,(DE)      ; A=(DE+37)
     PUSH      DE          
     POP       HL        
     DEC       HL          ; HL = DE+36
     AND       (HL  )      ; A &= (DE+36) 
     RLCA                  
     RLCA                  
     RLCA                  
     RRA                   
     JP        NC,0x038c    ; if( (DE+36) & 0x20 && (DE+37) & 0x20) return 2;
     LD        A,0x02       
     RET                   

L038C:
     LD        A,(DE)      ; A = (DE+37)
     RLCA                  
     RLCA                  
     RRA                   
     JP        NC,0x0396   ; if( (DE+37) & 0x40) return 0x01;
     LD        A,0x01       
     RET                   

L0396:
     LD        A,(DE)     ; A= (de+37)
     CPL                   
     AND       (HL  )     ; A = ~(de+37) & (de+36)
     RLCA                  
     RLCA                  
     RLCA                  
     RRA                   
     JP        NC,0x03a3  ; if( ~(de+37) & (de+36) & 0x20) return 0x0a;
     LD        A,0x0a       
     RET                   

L03A3:
     DEC       DE          
     LD        A,(DE)      ; A=(de+36)
     AND       0xfe         
     RRA                   
     RRA                   
     RRA                   
     JP        NC,0x03b0   ; if( (de+36) & 4) return 0x08;
     LD        A,0x08       
     RET                   

L03B0:
     LD        A,0xae      ; return 0xae;
     RET                   

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Kontrola poprawności zakresu parametrów funkcji odczytu (zapisu?) sektorów
;;
;; Funkcja sprawdza, czy parametry wejściowe funkcji odczytu sektora mają sens.
;; Funkcja zwraca kod błędu 8, jeśli stwierdzi, że:
;;   1. - ilość odczytywanych sektorów, przekracza długość ścieżki,
;;   2. - odczytywany sektor startowy nie mieści się w dopuszczalnym przedziale (1..160),
;;   3. - nr ścieżki jest poza zakresem,
;; jeśli parametry są prawidłowe, funkcja ustawia kod błędu na 0.
;;
;; wejście: DE - wskaźnik na obszar zmiennych
;; wyjście: A -> 0, jeśli wystąpił błąd, 1 jeśli parametry prawidłowe
;;
;; (DE+02) -> sects - ilość sektorów do odczytu
;; (DE+03) -> trk - numer ścieżki i głowicy 
;; (DE+04) -> sec0 - początkowy numer sektora
;; (DE+1a) -> err - kod błędu
;;
;; MAX_SECT -> (T0254) /* 0xa0 */
;; MIN_TRK  -> (T0255) /* 0x00 */
;;
;;char test_rd_params(char *de_ptr)
;;{
;;    if( ((sec0 + sects - 1) > MAX_SECT ) || (trk < MIN_TRK) || (sec0 < 1) ){
;;	err = 8
;;	return 0;
;;    }
;;    return 1;
;;}
;;

L03B3:
     LD        HL,0x0003     
     ADD       HL,DE     
     LD        A,(HL  ) 	; 
     LD        HL,T0255     
     SUB       A,(HL  ) 	; 
     SBC       A,A         	; 
     CPL                   	; A = (DE+3) > (T0255) ? 0x00:0xff
     LD        BC,0x0004     
     PUSH      DE          
     POP       HL        	;
     PUSH      HL        
     ADD       HL,BC     	;
     PUSH      HL        
     PUSH      DE          
     POP       HL        	; 
     INC       HL        
     INC       HL        	;
     POP       DE          	; HL = DE + 2, DE = DE + 4
     PUSH      AF          	; zachowaj A
     LD        A,(DE)      
     ADD       A,(HL  ) 	; 
     DEC       A           
     LD        C,A         	; C = (DE+4) + (DE+2) - 1
     LD        A,(T0254)    
     SUB       A,C         
     SBC       A,A         	; A = ((DE+4) + (DE+2) - 1) > (T0254) ? 0xff : 0x00
     POP       BC          	; B = (DE+3) > (T0255) ? 0x00:0xff
     LD        C,B         
     OR        C           	; ( A = A | B )----> B
     PUSH      AF          
     LD        A,(DE)      	; 
     SUB       A,0x00       	;
     SUB       A,0x01       	; 
     SBC       A,A         	; A = (DE+4) < 1 ? 0xff: 0x00
     POP       BC          
     LD        C,B         
     OR        C           	; A = A | B
     RRA                   
     POP       DE          
     JP        NC,0x03f0     	; jeśli !A  goto 3f0
     LD        HL,0x001a     
     ADD       HL,DE     
     LD        (HL   ),0x08  
     LD        A,0x00       
     RET                   
L03F0:
     LD        A,0x01       
     RET                   

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Funkcja obsługi błędów po operacji FDC READ/WRITE DATA
;;
;; Dane wejściowe:
;;  DE - wskaźnik obszaru zmiennych
;;  (DE+1a) - kod błędu 0
;;  (DE+12) - kod błędu 1 ?
;;  (DE+13) - kod błędu 2 ?
;;
;; Zwracany wynik:
;; (DE+33) - kod błędu, jeśli nie było błędu zwróć 0
;;

L03F3:
     LD        HL,0x001a     
     ADD       HL,DE     
     LD        A,(HL  ) 	; A = (DE+1a) - kod błędu po operacji FDC
     CP        0x00         
     JP        NZ,0x0406     	; jeśli wystąpił błąd skocz do L0406
     LD        HL,0x0033     
     ADD       HL,DE     
     LD        (HL   ),0x00  	; (DE+33) = 0; wyjdź z kodem błędu 0
     JP        0x041f        

L0406:
     CP        0x02         
     JP        NZ,0x0418        ; czy błąd FDC=2 ? jeśli nie skocz do 418
     PUSH      DE          
     CALL      0x02ee        	; wylicz błąd
     POP       DE          
     LD        HL,0x0033     
     ADD       HL,DE     
     LD        (HL  ),A 	; (DE+33) = funkcja_bledu();
     JP        0x041f        

L0418:				; wystąpił błąd, ale jest różny od 2 
     OR        0x80         
     LD        HL,0x0033     
     ADD       HL,DE     
     LD        (HL  ),A 	; (DE+33) = (DE+1a) | 0x80

L041F:
     RET                   

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Funkcja odczytu (zapisu ?) sektorów.
;;
;; Wykonuje wpierw rozkaz SEEK na napędzie disk, ustawiając głowicę na właściwej ścieżce trk, 
;; a następnie dokonuje odczytu (zapisu ?) nsect sektorów.
;;
;; DE - wskaźnik na obszar zmiennych
;; C  - kod operacji odczytu (zapisu ?)
;;
;; parametry wejściowe funkcji:
;; (DE+00) - numer napędu                -> disk
;; (DE+02) - ilość sektorów dla operacji -> nsect
;; (DE+03) - numer ścieżki i głowicy     -> trk
;; (DE+04) - początkowy numer sektora    -> sector
;; (DE+05,06) - adres bufora odczytywanych sektorów
;;
;; parametry zwracane:
;; (DE+1a) - kod błędu, jeśli złe parametry to 8
;; (DE+2f) - wynik operacji seek dla napędu 0 -> sres[0]
;; (DE+30) - wynik operacji seek dla napędu 1 -> sres[1]
;;
;; zmienne używane wewnętrznie:
;; (DE+07,08) -> tmp - kopia bufora odczytywanych sektorów
;; (DE+09...11) - kolejne parametry komendy odczytu ->  opc[0...8]
;; (DE+12) - kod błędu po operacji FDC READ/WRITE DATA -> rerr
;; 
;;

L0420:
     PUSH      BC          
     PUSH      DE          
     CALL      0x03b3        ; test parametrów wejściowych
     POP       DE          
     POP       BC          
     RRA                   
     JP        C,0x042c      ; if(!test_rd_params(DE)) return;
     RET                   

L042C:
     PUSH      BC          
     LD        HL,0x0003     
     ADD       HL,DE     
     LD        A,(HL  )		
     LD        HL,0x0000     
     ADD       HL,DE     
     LD        L,(HL  ) 	
     LD        H,0x00       	
     LD        BC,0x002f     
     ADD       HL,BC     
     ADD       HL,DE     	
     CP        (HL  )   
     JP        Z,0x0459      	; if( sres[disk] == trk) goto L0459
     LD        HL,0x0000     
     ADD       HL,DE     
     LD        B,(HL  ) 	; B = disk   /* numer napędu */
     LD        C,A         	; C = trk    /* numer cylindra i głowicy */
     PUSH      DE          
     CALL      0x0299        	; fdc_seek(B,C,DE);
     POP       DE          
     LD        HL,0x001a     
     ADD       HL,DE     
     LD        A,(HL  ) 
     CP        0x00         
     JP        Z,0x0459 	; if( serr != 0 ) return;
     POP       BC          
     RET                   

; rozkaz SEEK wykonał się pomyślnie
L0459:
     LD        HL,0x0005     
     ADD       HL,DE     	 
     LD        C,(HL  ) 	
     INC       HL        	
     LD        B,(HL  ) 	
     LD        HL,0x0007     
     ADD       HL,DE     
     LD        (HL  ),C 	
     INC       HL        
     LD        (HL  ),B 	; kopia początku adresu bufora, na operację odczytu
     INC       HL        	
     POP       BC          

; parametry wejściowe komendy
     LD        A,(T0259)    	; 
     OR        C           	;
     PUSH      HL        
     LD        HL,0x0260     
     OR        (HL  )   	;
     POP       HL        	; /* opcod rozkazu */ 
     LD        (HL  ),A 	; opc[0] = opcode | (T0259) | (C0260) ; MT MFM 0 0 opcode
     PUSH      DE          	; adres bufora
     PUSH      BC          
     LD        A,(DE)      
     LD        B,A         	; 
     INC       DE          
     INC       DE          
     INC       DE          	
     LD        A,(DE)      	
     LD        C,A         	; 
     INC       HL               ; 
     LD        A,C         	; 
     AND       0x01         	; 
     ADD       A,A         
     ADD       A,A         	; 
     OR        B           	; dana 1
     LD        (HL  ),A 	; opc[1] = A = disk | ((trk & 0x01) << 2) ; 00000 HDS DS1 DS0
     INC       HL        	; 
     OR        A           
     LD        A,C         
     RRA                   	; numer cylindra 'C'
     LD        (HL  ),A 	; opc[2] = (trk >> 1) | ( A ? 0x80:0x00)
     INC       HL        
     LD        A,C         
     AND       0x01         	; numer głowicy
     LD        (HL  ),A 	; opc[3] = trk & 0x01
     POP       BC          
     INC       DE              	;
     LD        A,(DE)          	;
     INC       HL              	; numer sektora 
     LD        (HL  ),A        	; opc[4] = sector
     LD        A,(0x0258)      	; A = 0x02
     INC       HL        	; ilość bajtów na sektor
     LD        (HL  ),A 	; opc[5] = (T0258) 
     LD        A,(0x0254)    
     INC       HL        	; pole EOT
     LD        (HL  ),A 	; opc[6] = 0xa0
     LD        A,(0x025a)    
     INC       HL               ; pole GPL
     LD        (HL  ),A 	; opc[7] = 0x12
     INC       HL        
     LD        A,(0x0258)       ; pole N
     CP        0x00         	; if( (T0258) != 0 ) goto L04b1
     JP        NZ,0x04b1     
     LD        (HL   ),0x80  	; opc[8] = 0x80 ; pole DTL
     JP        0x04b3        
L04B1:
     LD        (HL   ),0xff     ; opc[9] =0xff ; pole DTL
L04B3:
     POP       DE          	; 

; bufor rozkazu przygotowany, zawiera wszystkie 9 bajtów, czas na odczyt poszczególnych sektorów
; w pętli
     LD        HL,0x0002   
     ADD       HL,DE     	;
     LD        C,(HL  ) 	; C = nsect
L04B9:
     PUSH      DE          	; de - bufor
     PUSH      BC          
     LD        BC,0x0007     
     EX        DE,HL      
     ADD       HL,BC       	;
     LD        C,L         	; bc = de + 7 - adres bufora
     LD        B,H         
     CALL      0x099a        	; wysyła komendę do fdc

     POP       BC          
     POP       DE          
     LD        HL,0x0012     
     ADD       HL,DE     	
     LD        A,(HL  ) 	
     AND       0xd0         
     CP        0x00         
     JP        Z,L04D6     	; if(rerr != 0 ) return; /* bład wysłania komendy */
     JP        L04EA        
L04D6:

     DEC       C           	; C-- ; ilość sektorów -1
     JP        Z,0x04ea      	; if( C==0 ) return; jeśli wszystkie sektory odczytane wyjdź
     LD        HL,0x0008     
     ADD       HL,DE     
     LD        A,0x02       
     ADD       A,(HL  ) 	; 
     LD        (HL  ),A 	; tmp += 2 ; index bufora + 2, miejsce na kolejny sektor
     LD        HL,0x000d     
     ADD       HL,DE     
     INC       (HL  )   	; opc[4]++ ; następny sektor
     JP        0x04b9           ; odczyt następnego sektora

L04EA:
     RET                   

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; 
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; 
;;
;; L04EB
;;
;; Funkcja grupująca operacje FDC
;;
;; DE - adres bufora operacji. Adres ten nie jest modyfikowany i przekazany dalej
;; BC - adres zmiennych
;;
;; -----------------------------------------------------------------------------------------------
;; Funkcja kopiuje obszar zmiennych spod wskaźnika w BC pod wskaźnik w DE, a następnie
;; wykonuje skok względem numeru funkcji w (BC+1).
;;
;;                          -> Tablica skoków:
;; +0 *(DE + 0) = *(BC + 0) |  [0] L050B - L053B z pozytywnym zakończeniem i kodem błędu 00
;; +1 *(DE + 1) = *(BC + 1) -  [1] L0514 - wykonuje operację SEEK
;; +2 *(DE + 2) = *(BC + 2)    [2] L053B -> L053B
;; +3 *(DE + 3) = *(BC + 3)    [3] L053E - wykonuje operację RECALIBRATE
;; +4 *(DE + 4) = *(BC + 4)    [4] L0579 - wykonuje operację odczytu sektorów (READ_SECTORS)
;; +5 *(DE + 5) = *(BC + 5)    [5] L0583 -> L053B
;; +6 *(DE + 6) = *(BC + 6)    [6] L0586 - wykonuje operację zapisu sektorów (WRITE_SECTORS)
;;                             [7] L0590 -> L053B - koniec funkcji odczytu sektorów, kontrola błędów
;;
;; -----------------------------------------------------------------------------------------------
;; Parametry wejściowe:
;;
;; BC - adres bufora zmiennych z parametrami funkcji
;; DE - adres bufora roboczego funkcji
;;
;; *(BC + 00) - numer napędu, 0 - 'A', 1- 'B'
;; *(BC + 01) - kod operacji FDC (patrz tabelka wyżej)
;; *(BC + 02) - ilość sektorów 
;; *(BC + 03) - numer ścieżki
;; *(BC + 04) - numer pierwszego sektora
;; *(BC + 05) - adres bufora danych odczytu/zapisu, młodszy bajt
;; *(BC + 06) - adres bufora danych odczytu/zapisu, starszy bajt
;;
;; Parametry wyjściowe:
;;   A -> kod błędu operacji FDC
;;
L04EB:
     PUSH      DE          ; zachowaj DE
     PUSH      BC          
     EX        DE,HL       
     LD        E,0x07       
L04F0:
     LD        A,(BC)      ; for(e=7; e; e--)
     LD        (HL  ),A    ;     (DE++) = (BC++)
     INC       HL        
     INC       BC          
     DEC       E           
     JP        NZ,0x04f0     
     POP       HL          
     INC       HL        
     LD        A,0x07       
     AND       (HL  )       
     LD        C,A         
     LD        B,0x00       
     LD        HL,0x0593     
     ADD       HL,BC     
     ADD       HL,BC        
     LD        E,(HL  ) 
     INC       HL        
     LD        D,(HL  )     
     EX        DE,HL        
     POP       DE          ; odtwórz DE
     JP        (HL)        ; goto *(0x593 + 2*(*(pt2+1) & 0x7))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Wymuszenie pozytywnego zakończenia funkcji
;;
L050B:
 50b LD        HL,0x001a     ; wskaźnik zmiennej kodu błędu po operacji FDC
 50e ADD       HL,DE     
 50f LD        (HL   ),0x00  ; (DE + 0x001a) = 0
 511 JP        0x05a3        ; obsługa błędów

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Wykonanie operacji SEEK, wraz ze sprawdzeniem poprawności parametrów
;;
;; (DE+00) - nr napędu
;; (DE+03) - nr ścieżki i głowicy
;; (DE+1a) - kod błędu
;; *T0255 - min numer ścieżki
;;
L0514:
     LD        HL,0x0003     
     ADD       HL,DE     
     LD        A,(HL  ) 	; A = (DE+3)
     LD        HL,0x0255     
     CP        (HL  )   
     JP        C,0x0529         ; if( (DE+3) >= *T0255 ) goto 0x529 /* nie jestem pewny warunku */
     LD        HL,0x001a     
     ADD       HL,DE     
     LD        (HL   ),0x08     ; (DE+1a) = 0x08
     JP        0x0538 		; skocz do kontroli błędów       
L0529:
     LD        HL,0x0000     
     ADD       HL,DE     
     LD        B,(HL  )         ; B = (DE+00) ; - nr napędu
     LD        HL,0x0003     
     ADD       HL,DE     
     LD        C,(HL  )         ; C = (DE +3) ; - numer cylindra/głowicy
     PUSH      DE          
     CALL      0x0299           ; wykonaj SEEK/RECALIBRATE
     POP       DE          
L0538:
     JP        0x05a3           ; kontynuuj w kontroli błędów
L053B:
     JP        0x05a3        

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Wykonanie operacji RECALIBRATE
;; 
;; (DE+00) - numer napędu
;; (DE+2f) - kod błędu dla napędu 0
;; (DE+30) - kod błędu dla napędu 1
;;
L053E:
 53e LD        HL,0x0009     
 541 ADD       HL,DE     
 542 LD        (HL   ),0x07  	; (DE+09) = 7 - kod operacji RECALIBRATE
 544 LD        A,(DE)           ; 
 545 INC       HL        
 546 LD        (HL  ),A         ; DB (DE+0a) = (DE+00) - numer napędu
 547 LD        BC,0x0007     
 54a PUSH      DE          
 54b EX        DE,HL       
 54c ADD       HL,BC     
 54d LD        C,L         
 54e LD        B,H         	; BC = DE + 7
 54f CALL      0x099a           ; wykonaj komendę FDC RECALIBRATE
 552 POP       DE          
 553 LD        HL,0x001a     
 556 ADD       HL,DE     
 557 LD        A,(HL  ) 	; A=(DE+1a) - kod błędu po RECALIBRATE
 558 CP        0x00         
 55a JP        NZ,0x056b        ; if( *(DE+1a) !=0 ) goto 0x056b <- jeśli wystąpił błąd
 55d LD        A,(DE)      
 55e LD        L,A         
 55f LD        H,0x00      	 
 561 LD        BC,0x002f     
 564 ADD       HL,BC     
 565 ADD       HL,DE            ; 
 566 LD        (HL   ),0x00  	; (DE + 0x2f + (DE+00)) = 0; // wyzerowanie błędu napędu
 568 JP        0x0576        
; nie powiodła się komenda RECALIBRATE
L056B:
 56b LD        A,(DE)      
 56c LD        L,A         
 56d LD        H,0x00       
 56f LD        BC,0x002f     
 572 ADD       HL,BC     
 573 ADD       HL,DE     
 574 LD        (HL   ),0xff   	; (DE + 0x2f + (DE+00)) = 0; // ustawienie błędu napędu
L0576:
 576 JP        0x05a3           ; skok do obsługi błędu

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Funkcja odczytu sektorów READ_SECTORS
;;
;; Funkcja realizuje operację odczytu sektorów, wraz z ustawieniem na właściwą ścieżkę.
;;
;; DE - wskaźnik obszaru zmiennych
;;
;; parametry wejściowe funkcji:
;; (DE+00) - numer napędu                -> disk
;; (DE+02) - ilość sektorów dla operacji -> nsect
;; (DE+03) - numer ścieżki i głowicy     -> trk
;; (DE+04) - początkowy numer sektora    -> sector
;; (DE+05,06) - adres bufora odczytywanych sektorów
;;
;; parametry zwracane:
;; //(DE+1a) - kod błędu, jeśli złe parametry to 8
;; (DE+0x33) -
;;
L0579:
     LD        C,0x06	; kod operacji FDC: READ DATA
     PUSH      DE          
     CALL      0x0420	; operacja odczytu FDC: SEEK, READ DATA
     POP       DE          
     JP        0x05a3        
L0583:
     JP        0x05a3        

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Funkcja zapisu sektorów WRITE_SECTORS
;;
;; Funkcja realizuje operację zapisu sektorów, wraz z ustawieniem na właściwą ścieżkę.
;;
;; parametry wejściowe funkcji:
;; (DE+00) - numer napędu                -> disk
;; (DE+02) - ilość sektorów dla operacji -> nsect
;; (DE+03) - numer ścieżki i głowicy     -> trk
;; (DE+04) - początkowy numer sektora    -> sector
;; (DE+05,06) - adres bufora sektorów z danymi do zapisu
;;
;; //parametry zwracane:
;; //(DE+1a) - kod błędu, jeśli złe parametry to 8
;;
;;

L0586:
     LD        C,0x05       
     PUSH      DE          
     CALL      0x0420	; operacja zapisu FDC: SEEK, WRITE DATA
     POP       DE          
     JP        0x05a3        
L0590:
     JP        0x05a3        

; skoki do procedur 
T0593:
    DB 0x0b,0x05 ; (0) L050B - L05A3 z pozytywnym kodem błędu
    DB 0x14,0x05 ; (1) L0514 - SEEK
    DB 0x3b,0x05 ; (2) L053B -> L05A3
    DB 0x3e,0x05 ; (3) L053E - RECALIBRATE
    DB 0x79,0x05 ; (4) L0579 - odczyt sektorów
    DB 0x83,0x05 ; (5) L0583 -> L05A3
    DB 0x86,0x05 ; (6) L0586 -  zapis sektorów
    DB 0x90,0x05 ; (7) L0590 -> L05A3 - koniec funkcji odczytu sektorów, kontrola błędów

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Koniec wykonania funkcji READ_SECTORS i WRITE_SECTORS
;;
;; w A zwraca kod błędu operacji
;;

L05A3:
     PUSH      DE          
     CALL      0x03f3        ;
     POP       DE          
     LD        HL,0x0033     
     ADD       HL,DE     
     LD        A,(HL  )	     ; A = (DE+0x33)
     RET                   

;*******************************************************************************

 5ae LD        E,C         
 5af LD        D,B         
 5b0 PUSH      DE          
 5b1 CALL      0x0261        
 5b4 POP       HL        
 5b5 PUSH      HL        
 5b6 LD        BC,0x001b     
 5b9 ADD       HL,BC     
 5ba LD        C,L         
 5bb LD        B,H         
 5bc CALL      0x07d4        
 5bf POP       DE          
 5c0 LD        HL,0x0000     
 5c3 ADD       HL,DE     
 5c4 LD        (HL   ),0x00  
 5c6 LD        HL,0x0003     
 5c9 ADD       HL,DE     
 5ca LD        (HL   ),0x00  
 5cc LD        HL,0x002f     
 5cf ADD       HL,DE     
 5d0 LD        (HL   ),0x00  
 5d2 LD        BC,0x0000     
 5d5 CALL      0x0299        
 5d8 RET                   

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Ciąg dalszy RST #00 spod adresu #141
;;
;; Inicjowanie FDC, test czy jest to wersja dyskowa/bezdyskowa
;; 
;; Dane wejściowe
;; BC = 0x2000 - początek RAM, (0x3209) = 0
;;
;; Dane wyjściowe: brak
;;
L05D9:
     LD        A,0x0b       
     OUT       0xf1,A       ; włącz silniki obu napędów, RESET_FDC=0, TC_FDC=0
     PUSH      BC          
     POP       DE           ; DE = BC

; opóźnienie -czas potrzebny na pewne wyjście FDC 8272 z resetu, oraz reakcji FDD
     LD        BC,0x03e8    ; BC = 1000d
L05E2:			    ; do {
     DEC       BC           ;
     LD        A,C          ; A = C
     OR        B            ;
     JP        NZ,L05E2	    ; } while(BC & 0xff00)

     IN        A,0xee       ; pobierz rejestr statusu  FDC
     CP        0x80         ; czy można wysłać komendę dla FDC ?
     JP        Z,L05F9      ; jeśli tak to znaczy że mamy kontroler zainstalowany -> inicjowanie FDC
     CALL      0x0098       ; w przeciwnym razie wyświetl komunikat "wersja bezdyskowa"

L05F2:			    ; zakończenie funkcji
     LD        A,0xc8       ; 
     OUT       0xf1,A       ; wyłącz silniki FDD
     OUT       0xf7,A       ; powrót do zxrom ???
     RET                    ; koniec procedury RST #00

; mamy kontroler, więc teraz inicjowanie FDC
L05F9: 
     PUSH      DE           ; zachowaj parametr wejściowy z BC na stosie  - początek obszaru RAM
     CALL      L0261        ; wysłanie komendy SPECIFY do kontrolera FDC, ustawienie transmisji bez DMA
     POP       HL           ; HL - początek RAM
     PUSH      HL           ; kopia na stos
     LD        BC,0x001b    ;
     ADD       HL,BC        ;
     LD        C,L          ;
     LD        B,H          ; BC = HL + 0x1b - adres bufora wynikowego
     CALL      0x07d4	    ; wysyła komendę SENSE INTERRUPT STATUS do FDC
     POP       DE           ; DE - początek RAM
     LD        HL,0x0000    ; 
     ADD       HL,DE        ; 
     LD        (HL   ),0x00 ; (DE + 0000) = 0
     LD        HL,0x0003    ;
     ADD       HL,DE        ;
     LD        (HL   ),0x00 ; (DE + 0003) = 0
     PUSH      DE           ; i znowu zapamiętujemy parametr wejściowy z BC
     LD        BC,0x0000    ;
     CALL      0x0299       ; wysyła komendę RECALIBRATE DISK 0
     POP       DE           ; DE - parametr wejściowy z BC
     LD        BC,0x0100    ;
     CALL      0x0299       ; wysyła komendę RECALIBRATE DISK 1
     CALL      0x0090       ; wyświetl komunikat "wersja dyskowa"
     JP        L05F2        ; zakończ funkcję 

T0629:
    DB  0x00,0x00
    DB  0xff	; READ_TRACK
    DB  0x00	; SPECIFY
    DB  0x00	; SENSE DRV
    DB  0xff	; WRITE DATA
    DB  0xff	; READ DATA
    DB  0xff	; RECALIBRATE
    DB  0xff	; SENSE INT STAT
    DB  0xff	; WRITE DEL DATA
    DB  0xff	; READ ID
    DB  0x00	
    DB  0xff	; READ DEL DATA
    DB  0xff	; FORMAT TRACK
    DB  0x00
    DB  0xff	; SEEK
    DB  0x00
    DB  0xff	; SCAN EQ
    DB  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
    DB  0xff	; SCAN LE
    DB  0x00, 0x00, 0x00
    DB  0xff	; SCAN HE
    DB  0x00, 0x00

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Tablica ilości bajtów rozkazu do wysłania 
;;
;;       
T0649:
TAB_01:
    DB 0x00,0x00
    DB 0x09	; READ_TRACK
    DB 0x03	; SPECIFY
    DB 0x02	; SENSE DRV
    DB 0x09	; WRITE DATA
    DB 0x09	; READ DATA
    DB 0x02	; RECALIBRATE
    DB 0x01	; SENSE INT STAT
    DB 0x09	; WRITE DEL DATA
    DB 0x02	; READ ID
    DB 0x00
    DB 0x09	; READ DEL DATA
    DB 0x06	; FORMAT TRACK
    DB 0x00
    DB 0x03	; SEEK
    DB 0x00
    DB 0x09	; SCAN EQ
    DB 0x00,0x00,0x00,0x00,0x00,0x00, 0x00
    DB 0x09	; SCAN LE
    DB 0x00,0x00,0x00
    DB 0x09	; SCAN HE
    DB 0x00,0x00

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;  Funkcja:
;;    Testuje rejestr statusowy czy można wykonać zapis do rejestru danych FDC
;;  
;;  Parametry wejściowe: brak
;;
;;  Parametry Wyjściowe:
;;  A = 0 - nie można wysłać danej (FDC znajduje się np w fazie RESULT)
;;  A = 1 - można wysłać daną
;;
;;  Modyfikuje: AF,BC
;;
L0669:
     LD        A,0x02       
     CALL      L1572       ; L1572(2) - opóźnienie
L066E:
     IN        A,0xee      ; test czy można robić zapis/odczyt do FDC
     AND       0x80         
     CP        0x00         
     JP        NZ,L067a    ; można  
     JP        0x066e      ; nie można - sprawdzaj tak długo, aż będzie można   
L067A:
     IN        A,0xee      ; test czy można zrobić zapis do rejestru danych FDC
     AND       0x40         
     CP        0x00         
     JP        NZ,L0686    ; wyjście z A=0;
     LD        A,0x01       
     RET                   ; A=1; return;
L0686:
     LD        A,0x00       
     RET                   ; A=0; return;

 689 RET                   

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Funkcja:
;;   Test rejestru statusowego, oczekiwanie na gotowość transmisji
;;
;; Wejście:brak
;;
;; Wyjście:
;;    A = 0 -> można pisać do rejestru danych FDC
;;    A = 1 -> należy odczytać rejestr danych FDC (np w wyniku fazy result FDC)
;;    A = 3 -> FDC jest w stanie oczekiwania (nie wykonuje rozkazu zapis/odczyt)
;;    
L068A:
     LD        A,0x02       
     CALL      0x1572        ; opóźnienie
     IN        A,0xee        ; pobierz rejestr główny statusu FDC
     AND       0x10          ; bit CB - FDC jest w trakcie wykonywania rozkazu odczytu/zapisu
     CP        0x00          ; 
     JP        NZ,L069B      ; wykonywany jest rozkaz zapis/odczyt -> testuj dalej
     LD        A,0x03        ; jeśli nie wykonuje rozkazu zapis/odczyt -> A=3; return
     RET                   
L069B: 
     IN        A,0xee        ; pobierz rejestr główny statusu FDC
     AND       0x80          ; bit RQM - rejestr danych FDC jest gotowy do przyjęcia/wysłania danej
     CP        0x00         
     JP        NZ,L06A7      ; jest gotowy - > testuj dalej
     JP        0x069b        ; testuj dopóki będzie gotowy
L06A7:
     IN        A,0xee        ; pobierz rejestr główny statusu FDC
     AND       0x40          ; BIT DIO - wskazuje kierunek transmisji danych CPU <-> FDC
     CP        0x00         
     JP        Z,L06B3       ; można pisać do rejestru danych -> A=0; return
     LD        A,0x01        ; należy odczytać rejestr danych -> A=1; return
     RET                   
L06B3:
     LD        A,0x00       
     RET                   
L06B6:
     RET                   

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;  Funkcja:
;;	Wysyła daną do rejestru danych FDC
;;
;;  Parametry wejściowe:
;;      C - kod rozkazu
;;  Parametry wyjściowe:
;;      A = 0 - nie powiodło się (nie można w tej chwili wysyłać (np FDC jest w fazie RESULT )
;;	A = 1 - dana wysłana
;;
;;  Modyfikuje:
;;	AF,HL,BC
;;

L06B7:
     LD        B,C          ; 
     PUSH      BC           ; parametr wejściowy na stos
     INC       SP           ; SP teraz wskazuje na 
     CALL      L0669        ; A=L0669(); funkcja testuje czy można wysłać daną
     RRA                   
     JP        C,L06C5      ; if(A & 1) goto L06C5 -> można wysłać
     LD        A,0x00       
     INC       SP           ; wskaźnik stosu jak przed wykonaniem push BC
     RET                    ; A=0; return;
L06C5:
     LD        HL,0x0000     
     ADD       HL,SP     
     LD        A,(HL  )   
     OUT       0xef,A      ; wyślij komendę do FDC - numer komendy jako parametr wejściowy w C 
     LD        A,0x01       
     INC       SP          ; wskaźnik stosu jak przed wykonaniem push BC
     RET                   ; A=1; return; 


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 
;; Funkcja:
;;    Pobranie danej z FDC w fazie RESULT, jeżeli dana znajduje się w rejestrze FDC
;;
;; Wejście:
;;   BC - adres bufora, gdzie należy zapisać daną z fazy RESULT FDC "parm"
;;
;; Wyjście:
;; A = 0 -> FDC jest w stanie przyjmowania rozkazu - można słać daną do FDC
;; A = 1 -> dana odebrana w buforze pod adresem wskazywanym przez BC
;; A = 3 -> FDC jest w stanie oczekiwania (nie wykonuje rozkazu R/W)
;;
L06D0:
     DEC       SP            ; zmienna tymczasowa na stosie (sp+2) -> tmp
     PUSH      BC            ; parametr z BC na stos (sp+0) -> parm
     CALL      L068a         ; Test rejestru statusowego FDC, oczekiwanie na zakończenie rozkazu R/W
     LD        HL,0x0002     
     ADD       HL,SP     
     LD        (HL  ),A      ; tmp = A - wynik operacji Testu FDC
     LD        HL,0x0002     
     ADD       HL,SP     
     LD        A,(HL  )      ; A = tmp 
     RRA                   
     JP        C,L06E8      ; jeśli nie można pisać danej do rejestru FDC skocz dalej
     LD        A,0x00       ; gotowość do wysłania kolejnej danej
     INC       SP           
     POP       HL        
     RET                    ; A=0; return - można wysłać kolejną daną do FDC

L06E8:
     LD        HL,0x0002    
     ADD       HL,SP     
     LD        A,(HL  )     ; A = tmp - pobranie wyniku testu
     CP        0x03         ; nie wykonuje rozkazu R/W ?
     JP        NZ,0x06f7    ; jeśli wykonuje, tzn że faza RESULT i transmisja CPU<-FDC
     LD        A,0x03       
     INC       SP          
     POP       HL        
     RET                    ; A=3; return

; Faza RESULT rozkazu - należy odczytać rejestr danych FDC
L06F7:
     IN        A,0xef       ; odczytaj rejestr danych FDC
     LD        HL,0x0000     
     ADD       HL,SP     
     LD        C,(HL  ) 
     INC       HL        
     LD        B,(HL  )     
     LD        H,B         
     LD        L,C          
     LD        (HL  ),A     ; (parm) = A - zapisanie danej do bufora
     LD        A,0x01       
     INC       SP          
     POP       HL        
     RET                    ; A=1; return - dana jest w buforze, wskazywanym przez parm

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;  Wysłanie rozkazu do FDC
;;
;;
;; BC - par1 - wskaźnik
;;
;;
;;
;;
;;
L0708:
 708 DEC       SP          	;  miejsce na stosie dla zmiennej tymczasowej (SP+2) - i
 709 PUSH      BC          	;  parametr z BC na stos "par1"
 70a LD        HL,0x0002     
 70d ADD       HL,SP     
 70e LD        (HL   ),0x00  	; i=0 - zainicjowanie zmiennej

L0710E:
 710 LD        HL,0x0000     
 713 ADD       HL,SP     
 714 LD        C,(HL  ) 	
 715 INC       HL        
 716 LD        B,(HL  ) 	; par1
 717 INC       BC          
 718 INC       BC          	; 
 719 LD        A,(BC)      	;  
 71a AND       0x1f         	; A = *(par1+2) & 0x1f - pobranie danej spod wskaźnika/ ogranicznie wart do 5 bitów
 71c LD        C,A         	
 71d LD        B,0x00       	
 71f LD        HL,0x0649     
 722 ADD       HL,BC     	; HL = 0x649 + A ; a=0...31
 723 LD        A,(HL  ) 	; A = *HL; - ilość danych do przesłania ?
 724 DEC       A           	
 725 LD        HL,0x0002     
 728 ADD       HL,SP     
 729 CP        (HL  )   	 
 72a JP        C,0x0756      	; if(i > A) goto L0756 - wyjdź z A=1
 72d LD        HL,0x0002     
 730 ADD       HL,SP     
 731 LD        C,(HL  ) 	; C = i	
 732 LD        B,0x00       	; BC = C
 734 LD        HL,0x0002     
 737 ADD       HL,BC     	; HL = BC +2
 738 PUSH      HL        	
 739 LD        HL,0x0002     
 73c ADD       HL,SP     	; HL = SP+2
 73d POP       DE          	; DE = BC +2
 73e CALL      0x1569        	; HL = DE + (HL); -> HL=BC+2 + i; -> HL=i+2+i; -> HL=2*i+2;
 741 LD        C,(HL  ) 	; C  = (2*i+2) - kod danej
 742 CALL      0x06b7        	; Wysłanie do rejestru danych FDC kodu z C
 745 RRA                   
 746 JP        C,0x074e      	; jeśli udało się wysłać daną
 749 LD        A,0x00       
 74b INC       SP          
 74c POP       HL        
 74d RET                   	; A=0; return; - nie udało się wysłać 

L074E:
 74e LD        HL,0x0002     
 751 ADD       HL,SP     
 752 INC       (HL  )   	; 
 753 JP        NZ,L0710     	; if( BRYTE PTR ++i ) goto L0710
L0756:
 756 LD        A,0x01       
 758 INC       SP          
 759 POP       HL        
 75a RET                   	; A=1; return;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Pobranie danych z fazy result 
;;
;; 
;; BC - param_1
;;
;; A = 0 - pobrano wszystkie dane, można wysyłać nowe dane do FDC
;; A = 1 - oczekiwanie na koniec operacji W/R, nie można pobrać danej
;;
L075B:
     DEC       SP          	; (sp+4) - tmp_0  - wynik funkcji pobierającej daną
     PUSH      HL        	; (sp+3) - tmp_1  - odczytana dana 
    				; (sp+2) - tmp_2  - licznik pobranych bajtów
     PUSH      BC          	; (sp+0) - param_1 
     LD        HL,0x0002     
     ADD       HL,SP     	; 
     LD        (HL   ),0x00  	; tmp_2=0;
L0764:
     LD        A,0x07       	;
     LD        HL,0x0002     	; 
     ADD       HL,SP     	; 
     CP        (HL  )   	; 
     JP        C,L07BB      	; if( tmp_2 < 7) goto 7bb
     LD        HL,0x0003     
     ADD       HL,SP     
     LD        B,H         	; BC = &tmp_1 - tmp1 będzie zawierać daną odczytaną z FDC
     LD        C,L         	; BC - adres bufora wyjściowego
     CALL      L06D0        	; Pobranie danej z fazy RESULT, dana znajduje się w (BC)
     LD        HL,0x0004     
     ADD       HL,SP     
     LD        (HL  ),A 	; tmp_0 = A - wynik funkcji pobierania danej
     
     LD        HL,0x0004     
     ADD       HL,SP     
     LD        A,(HL  ) 	; A = tmp_0
     RRA                   
     JP        C,L078b      	; if(tmp != 0) goto 78b - dana pobrana, lub nie można było pobrać
     LD        A,0x00           ; nie ma więcej danych do pobrania
     INC       SP          	
     POP       HL        
     POP       HL        
     RET                   	; A = 0; return

L078B:
     LD        HL,0x0004     
     ADD       HL,SP     
     LD        A,(HL  ) 	; A = tmp_0
     CP        0x03         	; 
     JP        NZ,L079B     	; if(A != 3) goto 79B -> dana pobrana, czeka w tmp_1
     LD        A,0x01           ; nie można było pobrać danej - FDC w stanie oczekiwania
     INC       SP          
     POP       HL        
     POP       HL        
     RET                   	; A = 1; return; - oczekiwanie na koniec operacji W/R
;; dana została odebrana i czeka w tmp_1, należy ją skopiować do bufora, i pobrać następną
;; :| ?????
L079B:
     LD        HL,0x0002        
     ADD       HL,SP     
     LD        C,(HL  ) 	; C = tmp_2 
     LD        B,0x00       	; BC = C 

     LD        HL,0x000b        
     ADD       HL,BC     	; HL = &param_1[0x0b] ??
     PUSH      HL        	
     LD        HL,0x0002        ; - hmmm, gdyby było 0000, to by się wszystko zgadzało
     ADD       HL,SP     	; HL = &tmp_2 = SP+2
     POP       DE          	; DE = tmp_2 + 11 
     CALL      0x1569        	; HL = DE + (HL+0) + 256*(HL+1); DE= HL+1=SP+3
     INC       DE          	; SP+4
     INC       DE          	; SP+5 ; ????
*     LD        A,(DE)      	
     LD        (HL  ),A 	; (HL) = (DE) 
     LD        HL,0x0002     
     ADD       HL,SP     
     INC       (HL  )   	; tmp_2++
     JP        NZ,0x0764     	; if(tmp_2 !=0 ) goto L0764 - następna dana do odczytu
L07BB:
     IN        A,0xee       	;
     AND       0x10         	;
     CP        0x00         	
     JP        Z,0x07ca      	;
     LD        A,0x00       
     INC       SP          
     POP       HL        
     POP       HL        
     RET                   	; A=0; return
 
L07CA:
     LD        A,0x01       
     INC       SP          
     POP       HL        
     POP       HL        
     RET                   	; A=1; return

 
 7d0 INC       SP          
 7d1 POP       HL        
 7d2 POP       HL        
 7d3 RET                   
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Wysyła komendę SENSE INTERRUPT STATUS do kontrolera FDC, oraz zapamiętuje wynik
;; komendy w buforze wskazywanym przez BC
;;
;; BC = 201b - adres bufora wynikowego 
;;
L07D4:
     PUSH      BC          	; tmp = BC  (sp+0)
L07D5:
     LD        DE,0x000b        ; 
     LD        HL,0x0000        ; 
     ADD       HL,SP     	; HL = SP+2
     CALL      0x1569           ; HL = tmp + 0x000b
     LD        A,(HL  ) 	; A = (HL) - dana z bufora wynikowego
     AND       0xe0         	; maska abc00000 CY=0
     RLA                   	; A = bc00000C CY = a
     RLA                   	; A = c00000Ca CY = b
     RLA                   	; A = 00000Cab CY = c
     CP        0x02         	; A ? 2
     JP        Z,0x080a         ; if (A == 2) return

     LD        C,0x08       	; kod komendy SENSE INTERRUPT STATUS
     CALL      0x06b7           ; wyślij komendę do kontrolera FDC
     RRA                   
     JP        C,0x07f6         ; jeśli się udało idź dalej
     JP        0x07d5           ; powtórz próbę wysłania

; rozkaz wysłany, więc teraz należy pobrać wynik z fazy result
L07F6:
     LD        HL,0x0000 	;    
     ADD       HL,SP     	; 
     LD        C,(HL  ) 	; 
     INC       HL        	; 
     LD        B,(HL  ) 	; BC = (SP+0)
     CALL      0x075b        	; pobranie danych z fazy RESULT FDC
     RRA                   
     JP        C,0x0807         
     JP        0x07d5           ; powtórz próbę wysłania rozkazu do FDC
L0807: 
     JP        0x07d5           ; powtórz próbę wysłania rozkazu do FDC

L080A:
     POP       HL        
     RET                   
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Przekopiowanie 7 (?) bajtów z obszaru parm1+11 do parm2+11,
;;  ustawienie: parm2[0x12]=1;
;; 
;; Wejście:
;; 	DE- parm1
;; 	BC- parm2
;; Wyjście:
;;	parm2[0x12] = 1
;;
;; for(tmp=0; tmp <= 6; tmp++ ) de[ 11 + tmp] = bc[ 11 + tmp]; bc[0x12] = 1;
;;
L080C:
     DEC       SP          	; (sp+4) = tmp
     PUSH      DE          	; (sp+2) = parm1
     PUSH      BC          	; (sp+0) = parm2
     LD        HL,0x0004     
     ADD       HL,SP     
; skopiowanie 6 bajtów z obszaru parm1+11 do parm2+11
     LD        (HL   ),0x00  	; tmp = 0
L0815: 
     LD        A,0x06       
     LD        HL,0x0004     
     ADD       HL,SP     
     CP        (HL  )   	; 
     JP        C,L0849      	; if(tmp > 6) goto L0849
     LD        HL,0x0004     
     ADD       HL,SP     
     LD        C,(HL  ) 	; 
     LD        B,0x00       	; BC = tmp
     LD        HL,0x000b     
     ADD       HL,BC
     PUSH      HL        
     LD        HL,0x0004     
     ADD       HL,SP
     POP       DE   
     CALL      L1569
     PUSH      HL
     LD        HL,0x000b     
     ADD       HL,BC
     DEC       DE          	
     DEC       DE          	 
     DEC       DE          	
     CALL      0x156a        	; HL = &parm2[0x0b + tmp]
     POP       BC		; BC = &parm1[0x0b + tmp]
     LD        A,(BC)      
     LD        (HL  ),A 	; parm2[0x0b + tmp] = parm1[0x0b + tmp] ; kop poj bajtów
     LD        HL,0x0004     
     ADD       HL,SP     
     INC       (HL  )   	; tmp++
     JP        NZ,0x0815        ; if( tmp != 0) goto 815 - następny bajt do kopiowania

L0849:
     LD        DE,0x0012     
     LD        HL,0x0000     
     ADD       HL,SP     	
     CALL      0x1569        
     LD        (HL   ),0x01  	; parm2[0x12] = 1
     INC       SP          
     POP       DE          
     POP       DE          
     RET                   

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Wykonanie SENSE INTERRUPT STATUS po operacji SEEK lub RECALL
;; BC - parm2 - bufor wynikowy
;; HL - parm1 - bufor odczytu
;; >>  wykonuje się gdy CB=0 << 
;;
L0859:
     DEC       SP          	; (sp+4) - tymczasowa zmienna tmp_0
     PUSH      HL        	; (sp+2) - parm1 
     PUSH      BC          	; (sp+0) - parm2

     LD        A,0x13           ;
     LD        HL,0x0000     	;
     ADD       HL,SP     	; 
     CALL      L1566        	;
     EX        DE,HL       	;
     INC       HL        	;
     LD        (HL  ),E 	;
     INC       HL        	;
     LD        (HL  ),D 	; param2[0x00] = &param2[0x13] - wskaźnik wyniku operacji

     IN        A,0xee       	; pobierz rejestr statusowy
     AND       0x10         	; testuj bit CB rejestru MSR
     CP        0x00         
     JP        Z,L089F      	; jeśli CB == 0, wykonuj dalej pod adresem L089F
; FDC jest w trakcie operacji R/W - faza result, należy odczytać dane z bufora danych FDC
     LD        HL,0x0000     	
     ADD       HL,SP     
     LD        C,(HL  ) 	
     INC       HL        
     LD        B,(HL  ) 	; BC = parm2
     CALL      L075B        	; pobranie danych z fazy RESULT
     RRA                   
     JP        C,L0890      	; nie udało się pobrać wszystkich danych
; wszystkie dane pobrano z fazy result
     LD        DE,0x0012     	; 
     LD        HL,0x0000     
     ADD       HL,SP     	 
     CALL      L1569        	; oblicz adres tablicy
     LD        (HL   ),0x00  	; parm2[0x12] = 0x00
     JP        L089C            ; i dalej pod 89c
L0890: 
     LD        DE,0x0012     
     LD        HL,0x0000     
     ADD       HL,SP     
     CALL      0x1569        
     LD        (HL   ),0x01     ; parm2[0x12] = 0x01
L089C:
     JP        0x0996           ; oczyszczenie stosu i return
; FDC jest w stanie oczekiwania, można wysłać kolejna komendę
L089F:
     LD        A,0x05       
     CALL      0x1572        	; opóźnienie
     LD        HL,0x0004     
     ADD       HL,SP     
     LD        (HL   ),0x01     ; tmp_0 = 1
L08AA:
     LD        HL,0x0004     
     ADD       HL,SP     
     LD        A,(HL  ) 	
     RRA                   
     JP        NC,L094C        ; if tmp_0 & 1 == 0 goto 94c - wyjście z pętli
     LD        C,0x08       
     CALL      0x06b7          ; wysyła komendę SENSE IRQ STATUS FDC
     RRA                   
     JP        C,0x08cc        ; udało się wysłać  idź do 8cc
; nie udało się wysłać komendy SENSE IRQ STATUS
     LD        DE,0x0012     
     LD        HL,0x0000     
     ADD       HL,SP     
     CALL      0x1569        
     LD        (HL   ),0x00    ; parm2[0x12] = 0x00
     INC       SP          
     POP       HL        
     POP       HL        
     RET                       ; wyjdź
; SENSE IRQ STATUS wysłane, teraz należy odczytać dane z fazy RESULT 
L08CC:
     LD        HL,0x0002     
     ADD       HL,SP     
     LD        C,(HL  ) 
     INC       HL        
     LD        B,(HL  )      ; BC = parm1 - bufor odczytu 
     CALL      L075B         ; pobranie danych z fazy result
     RRA                   
     JP        C,0x08ea      ; jeśli się powiódł odczyt danych to skocz do 8EA
; jeśli nie to zakończ z kodem błędu
     LD        DE,0x0012     
     LD        HL,0x0000     
     ADD       HL,SP     
     CALL      0x1569        
     LD        (HL   ),0x00 ; parm2[0x12] = 0
     INC       SP          
     POP       HL        
     POP       HL        
     RET                    ; return
 
; faza RESULT komendy SENSE IRQ zakończona sukcesem
L08EA:
     LD        DE,0x000b     
     LD        HL,0x0002     
     ADD       HL,SP     
     CALL      0x1569        
     LD        A,(HL  )     ; A = parm1[0x0b] -> zawartość rejestru ST0 FDC
     AND       0xe0         ; abc00000 CY=0
     RLA                    ; bc00000C CY=a
     RLA                    ; c00000Ca CY=b
     RLA                    ; 00000Cab CY=c
     LD        C,A          
     LD        B,0x00       ; BC = 00000000000000ab
     LD        HL,0x0941    ; zawiera 4 wskaźniki do funkcji:
     ADD       HL,BC        ;            L0949, L0907,L0921,L093B
     ADD       HL,BC        ; HL = 2*BC + 0x941
     LD        E,(HL  ) 
     INC       HL        
     LD        D,(HL  )     ; DE = *HL
     JP        (HL)         ; JP HL

; skok dynamiczny z funkcji L0859 idx=1
L0907: 
     LD        HL,0x0000     
     ADD       HL,SP     
     LD        C,(HL  ) 
     INC       HL        
     LD        B,(HL  ) 		
     LD        HL,0x0002     
     ADD       HL,SP     
     LD        E,(HL  ) 	
     INC       HL        	; BC = parm2 (tablica zmiennych)
     LD        D,(HL  ) 	; DE = parm1 (bufor odczytu dla RESULT )
     CALL      L080C        	; ??
     LD        HL,0x0004     
     ADD       HL,SP     
     LD        (HL   ),0x00  	; tmp_0 = 0
     JP        0x0949        	; powtórz komendę SENSE IRQ STATUS FDC

; skok dynamiczny z funkcji L0859 idx=2
L0921: 
     LD        HL,0x0000     
     ADD       HL,SP     
     LD        C,(HL  ) 
     INC       HL        
     LD        B,(HL  ) 	
     LD        HL,0x0002     
     ADD       HL,SP     
     LD        E,(HL  ) 
     INC       HL        	; BC = parm2
     LD        D,(HL  ) 	; DE = parm1
     CALL      L080c        	; przekopiowanienie 7 bajtów z de[11+i]=bc[11+i]; bc[0x12]=1
     LD        HL,0x0004     
     ADD       HL,SP     
     LD        (HL   ),0x00  	; tmp_0 = 0x00 -> koniec pętli, skok pod 94C
     JP        0x0949           ; powtórz komendę SENSE IRQ STATUS FDC i wyjdź z pętli
; skok dynamiczny z funkcji L0859 idx=3
L093B: 
     JP        0x0949        ; powtórz komendę SENSE IRQ STATUS FDC
L093E:
     JP        0x0949        ; powtórz komendę SENSE IRQ STATUS FDC        

T0941:
    DW L0907,L0921,L093B,L093E
; skok dynamiczny z funkcji L0859 idx=0
L0949: 
 949 JP        0x08aa        ; powtórz komendę SENSE IRQ STATUS FDC

; po wykonaniu instrukcji RECALL/SEEK i następującej po niej SENSE IRQ


L094C:
 94c LD        HL,0x0004     
 94f ADD       HL,SP     
 950 LD        (HL   ),0x00   ; tmp_0 = 0
L0952:
 952 LD        HL,0x0004     
 955 ADD       HL,SP     
 956 LD        A,(HL  )       ; 
 957 RRA                   
 958 JP        C,L0996        ;if( tmp_0 & 1) goto 996
 95b LD        C,0x08          
 95d CALL      L06B7          ; wyślij komendę SENSE IRQ STATUS FDC
 960 RRA                   
 961 JP        C,0x0967       ; jeśli udało się wysłać goto 967
 964 JP        0x0996         ; inaczej wyjdź z funkcji CY = 0

; udało się wysłać komendę SENSE IRQ STATUS FDC
L0967:
 967 LD        HL,0x0002     
 96a ADD       HL,SP     
 96b LD        C,(HL  ) 
 96c INC       HL        
 96d LD        B,(HL  )       ; BC = parm_1 - wynik
 96e CALL      L075B          ; pobranie danej z fazy result
 971 RRA                   
 972 JP        C,0x0978       ; jeśli ok to skok do 978
 975 JP        0x0996         ; nie udało się pobrać danych, zakończ funkcję

; dane pobrane z fazy  RESULT
L0978:
 978 LD        DE,0x000b     
 97b LD        HL,0x0002     
 97e ADD       HL,SP     
 97f CALL      0x1569        
 982 LD        A,(HL  )        ; A = SP0 FDC
 983 AND       0xe0         
 985 RLA                   
 986 RLA                   
 987 RLA                   
 988 CP        0x02            ; A == 0x80 ?
 98a JP        NZ,0x0993       ; if( A!=0x80 ) goto 952 - powtórz wysłanie
 98d LD        HL,0x0004     
 990 ADD       HL,SP     
 991 LD        (HL   ),0x01    ; tmp = 1
 993 JP        0x0952          ; goto 952 - wyjdź z funkcji

L0996:
 996 INC       SP          
 997 POP       DE          
 998 POP       DE          
 999 RET                   

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Funkcja:
;;  Wysyła komendę do kontrolera FDC
;;  
;; Wejście:
;;  BC - param1 - adres bazowy dla zmiennych 
;;
;;  param1 + 2   - bufor komendy do wysłania
;;  param1 + 13h  - kod wyniku operacji
;;  [BC+00,01] - adres bufora odczytu/zapisu danych
;; 
L099A:
     PUSH      BC          	; param1 -> (SP+0)
     LD        HL,0x0000     
     ADD       HL,SP     	
     LD        C,(HL  ) 	; 
     INC       HL        
     LD        B,(HL  ) 	; BC= param1 - bufor komendy
     CALL      L0708        	; A = L0708() - wysłanie komendy do FDC
     RRA                   
     JP        C,L09B7    	; jeśli udało się wysłać bez błędu to skocz pod L09B7 (?)
     LD        DE,0x0013     	; 
     LD        HL,0x0000     	;
     ADD       HL,SP     	; 
     CALL      0x1569        	; 
     LD        (HL   ),0x03  	; param1[0x13] = 3 - kod błędu
     POP       HL        	; odtwórz parametr wejściowy z BC w HL
     RET                   	; return
; udało się prawidłowo wysłać komendę
L09B7:
     LD        HL,0x0000     
     ADD       HL,SP     
     LD        C,(HL  ) 
     INC       HL        
     LD        B,(HL  ) 	
     INC       BC          
     INC       BC       
     LD        A,(BC)      	
     AND       0x1f         	; A = param1[0x02] & 0x1f - kod instrukcji
     CP        0x03         
     JP        NZ,L09D6     	; if( param1[0x02] != FDC_SPECIFY) goto L09D6

; wykonywana instrukcja była instrukcją FDC SPECIFY
     LD        DE,0x0013     
     LD        HL,0x0000     
     ADD       HL,SP     
     CALL      0x1569        
     LD        (HL   ),0x00  	; param1[0x13] = 0x00
     POP       HL        
     RET                   	; return
; to nie była instrukcja FDC SPECIFY 
L09D6:
     LD        HL,0x0000     
     ADD       HL,SP     
     LD        C,(HL  ) 
     INC       HL        
     LD        B,(HL  ) 	
     INC       BC          
     INC       BC          
     LD        A,(BC)      	
     AND       0x1f         	; tmp = param1[0x02] & 1f - kod instrukcji
     PUSH      AF          
     SUB       A,0x06       
     SUB       A,0x01       
     SBC       A,A       	; aw7 = tmp == 6 (READ DATA)
     POP       BC          	
     LD        C,B         	
     PUSH      AF          	
     LD        A,C         	
     PUSH      AF          	
     SUB       A,0x05       
     SUB       A,0x01       
     SBC       A,A         	; aw6 = tmp == 5 (WRITE DATA)
     POP       DE          	
     LD        E,D         	
     POP       BC          	
     LD        C,B         	
     OR        C           	
     PUSH      AF          	
     LD        A,E         	
     SUB       A,0x0d       
     SUB       A,0x01       	
     SBC       A,A         	; A = aw14 = tmp == 13 (FORMAT A TRACK)
     POP       BC          	
     LD        C,B         	
     OR        C           	
; A = (FORMAT | READ | WRITE)
     RRA                   	; tmp - kod innstrukcji
     JP        NC,L0A1E        
; jeśli instrukcja to nie FORMAT | READ | WRITE goto L0A1E
     LD        HL,0x0000     
     ADD       HL,SP     
     LD        C,(HL  ) 
     INC       HL        
     LD        B,(HL  ) 	; BC = parm1
     INC       BC          
     INC       BC          
     LD        A,(BC)      	
     AND       0x1f         	; A = parm1[0x02] & 1f - kod instrukcji
     DEC       HL        
     LD        C,(HL  ) 
     INC       HL        
     LD        B,(HL  ) 	
     LD        H,B         
     LD        L,C         	
     LD        E,(HL  ) 
     INC       HL        
     LD        D,(HL  ) 	; DE = parm[0x00]
     LD        C,A         	; C - kod wykonanej instrukcji
     CALL      L0200        	; pobranie/wysłanie danych do FDC w operacji zapisu/odczytu sektoru

L0A1E:
     LD        HL,0x0000     
     ADD       HL,SP     
     LD        C,(HL  ) 
     INC       HL        
     LD        B,(HL  ) 	; BC = parm1
     CALL      L0859        	; próba wysłania  ISENSE INTERRUPT STATUS
     LD        DE,0x0012       
     LD        HL,0x0000     
     ADD       HL,SP     
     CALL      0x1569        
     LD        A,(HL  ) 	
     CP        0x00         
     JP        NZ,0x0a46     	; if(parm1[0x12] !=0) goto L0A46
     LD        DE,0x0013     
     LD        HL,0x0000     
     ADD       HL,SP     
     CALL      0x1569        
     LD        (HL   ),0x04  	; parm1[0x13] = 4
     POP       HL        
     RET                   

L0A46:
     LD        HL,0x0000     
     ADD       HL,SP     
     LD        C,(HL  ) 
     INC       HL        
     LD        B,(HL  ) 	
     INC       BC          
     INC       BC          
     LD        A,(BC)      	
     AND       0x1f         	
     LD        C,A         
     LD        B,0x00       	
     LD        HL,0x0629     	
     ADD       HL,BC     	
     LD        DE,0x000b     
     PUSH      HL        
     LD        HL,0x0002     
     ADD       HL,SP     
     CALL      0x1569        
     LD        A,(HL  ) 	
     POP       HL       
  
     AND       (HL  )   	; A = parm1[0x0b] & T0629[parm1[0x02]] & 0xd0
     AND       0xd0         
     CP        0x00         
     JP        NZ,0x0a7d     	; A!= 0 goto 7ad

     LD        DE,0x0013     
     LD        HL,0x0000     
     ADD       HL,SP     
     CALL      0x1569        
     LD        (HL   ),0x00  	; parm1[0x13] = 0x00
     JP        0x0a89        	; koniec funkcji

L0A7D:
     LD        DE,0x0013     
     LD        HL,0x0000     
     ADD       HL,SP     
     CALL      0x1569        
     LD        (HL   ),0x02     ; parm1[0x13] = 0x02
L0A89:
     POP       DE          
     RET                   

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
T0A8B:
    DB "\nKatalog dysku PRGARRSTRCOD"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; L0AA6
;;
;; Funkcja wykonuje operację: HL = 0x2080 + 32 * (A & 0x3f); 
;;
;; Wywoływana jest przy odczycie katalogu dyskowego.
;;
;; A  - pozycja w katalogu
;; HL - adres w buforze
;;
;; 0x2080 - adres bufora danych odczytanego klastra 
;;
L0AA6:
     AND       0x3f         ;
     LD        L,A          ;
     LD        H,0x00       ; HL = A & 0x3f
     ADD       HL,HL       
     ADD       HL,HL       
     ADD       HL,HL       
     ADD       HL,HL       
     ADD       HL,HL        ; HL = HL << 5;
     LD        BC,0x2080     
     ADD       HL,BC        ; HL = 0x2080 + 32 * (A & 0x3f); 
     RET                   

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; L0AB5
;;
;; Mnożenie HL przez 12 (długość nazwy pliku)
;; 
;; Funkcja wykonuje funkcję: HL = HL*12
;; 
L0AB5:
     ADD       HL,HL       ;
     ADD       HL,HL       ; HL * 4
     LD        D,H         
     LD        E,L         ; DE = HL *4
     ADD       HL,HL       ; HL * 2
     ADD       HL,DE       ; HL + DE
     RET                   

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; L0ABC
;;
;; 
;; 
;; Funkcja wykonuje funkcję: BC = 4 * A
;; 
L0ABC:
     RLCA                  ; A = A << 1
     RLCA                  ; A = A << 1
     AND       0x03        ; A & = 0x03 /* wyzerowanie 'zawiniętych' bitów */
     LD        C,A         ;
     LD        B,0x00      ; BC = A
     RET                   


 ac4 LD        (HL   ),0x24  
 ac6 INC       HL        
 ac7 LD        (HL   ),0x24  
 ac9 INC       HL        
 aca LD        (HL   ),0x24  
 acc RET                   

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Wykonanie operacji FDC na sektorze
;;
;; Dane wejściowe: 
;;   DE - adres bufora danych (2kB)
;;   BC - numer sektora logicznego (liczony od sektora 1 na ścieżce 1 )
;;   stos - kod funkcji (patrz funkcja 04EB )
;;   *(0x3209) - nr napędu
;; Dane wyjściowe:
;;   A -> kod błędu
;; 
;; Funkcja odczytuje (/zapisuje ? ) klaster z FDD (4 sektory - 2048b).
;; Jeśli podczas operacji wystąpił błąd, następuje ustawienie głowicy na ścieżce 0 i
;; operacja zostaje powtórzona. Po 10 nieudanych próbach funkcja kończy działanie z kodem błędu
;; A=0x00. W przypadku udanego odczytu klastra, w akumulatorze zostanie zwrócona wartość 0xff.
;;
;;
L0ACD:
     LD        HL,0x3220     
     LD        (HL  ),D 	
     DEC       HL        
     LD        (HL  ),E 	; (0x321f) = DE
     DEC       HL        
     LD        (HL  ),B 
     DEC       HL        
     LD        (HL  ),C         ; (0x321d) = BC
     DEC       HL               
     POP       DE               ; adres powrotu z procedury
     POP       BC               ; pobranie parametru funkcji do BC
     LD        (HL  ),C         ; (0x321c) = parametr
     PUSH      DE          	; odtworzenie adresu powrotu
     LD        HL,0x3209        
     LD        C,(HL  )         ; C = (3209) nr dysku
     CALL      0x01ed           ; załączenie silnika napędu
     LD        A,(0x3209)    
     LD        (0x2880),A       ; (2880) = nr napędu
     LD        A,(0x321c)    
     LD        (0x2881),A       ; (2881) = parametr /* kod operacji */
     LD        HL,0x2882     
     LD        (HL   ),0x01  	; (2882) = 0x01     /* ilość sektorów */
     LD        HL,(0x321d)   
     ADD       HL,HL       
     ADD       HL,HL       
     LD        (0x3221),HL   	; DW (3221) = (321d) + 2
     LD        HL,(0x321f)      
     LD        (0x2885),HL      ; DW (2885) = (321f) /* adres bufora danych */
     LD        HL,0x3224        ; 
     LD        (HL   ),0x00     ; (3224) = 0 początek pętli odczytu sektorów
L0B07:
     LD        A,(0x3224)       ; A=(3224)
     CP        0x04         
     JP        NC,0x0bb3        ; if( (3224) >= 4) goto L0BB3
; 
; poniższy fragment realizuje funkcję hl = (hl-1) / 9
;
     LD        HL,(0x3221)      ; HL=(3221)
     LD        BC,0x0000        ; BC=0
     PUSH      BC               ; BC na stos
     LD        DE,0x9000        ; DE = 0x9000
     LD        C,0x0d       	; C=0x0d licznik pętli
L0B1B:
     PUSH      HL        
     OR        A                ; 
     SBC       HL,DE     	; 
     POP       HL               ;
     JP        C,0x0b27      	; if(HL < DE ) goto b27 (cy=1)
     OR        A           
     SBC       HL,DE     	; HL -= DE
     OR        A           	; 
L0B27:
     CCF                   	; CY = !CY
     EX (SP),HL            
     RL        L           	; z xxxxxxxx xxxxxxxy : (stos)
     RL        H           
     EX (SP),HL            
     RR        D           
     RR        E           	;   zxxxxxxx xxxxxxxx y : DE
     DEC       C           
     JP        NZ,0x0b1b     
     POP       HL        	; HL = INT( (*(0x3221) - 1) / 9 )
;
; poniższy fragment realizuje funkcję HL = (HL % 9) + 2
;
     INC       HL        
     INC       HL        
     LD        A,L         
     LD        (0x2883),A  	; (2883) = {HL +2 }  & 0xff /* numer ścieżki */
     DEC       HL        
     DEC       HL        
     LD        B,H         
     LD        C,L         	; 
     ADD       HL,HL       
     ADD       HL,HL       
     ADD       HL,HL       
     ADD       HL,BC     	
     EX        DE,HL       	; de = hl*7
     LD        HL,(0x3221)   	; hl = (0x3221) 
     OR        A           
     SBC       HL,DE		; hl -= de
     INC       HL        	; hl++
     LD        A,L         
     LD        (0x2884),A       ; (0x2884) = hl & 0xff  - pierwszy sektor

; str.track = ((*(0x3221)-1) / 9 ) + 2;  /* początek katalogu to ścieżka 01, więc +2 (pierwszy bit to głowica )*/
; str.f_sec = ((*(0x3221)-1) % 9 ) + 1;
; i = 0;
; while( call_fdc(data_bf, str) && i <= 10 ){
;   str.kod = RECALIBRATE;
;   err = call_fdc(data_bf, str);
;   str.kod = stack_param;
;   i++;
; }
     LD        HL,0x3223     
     LD        (HL   ),0x00     ; (3223)=0x00 ; licznik pętli
L0B56:
     LD        DE,0x2000        ; DE = 2000 - adres bufora roboczego
     LD        BC,0x2880        ; BC = 2880 - adres parametrów
     CALL      0x04eb           ; 
     OR        A           
     JP        Z,L0B89          ; if(A==0) goto L0B89 ; jeśli operacja wykonała się z sukcesem wyjdź
     LD        A,(0x3223)       ; 
     CP        0x0a         
     JP        NC,L0B89         ; if(*(0x3223) > 10) goto L0B89 ; jeśli po 10 nieudanych próbach wyjdź
     LD        HL,0x2881     
     LD        (HL   ),0x03     ; (0x2881) = 3; RECALIBRATE
     LD        DE,0x2000     
     LD        BC,0x2880     
     CALL      0x04eb        	; odczyt pierwszej ścieżki
     LD        (0x3208),A       ; (0x3208) = kod błędu operacji
     LD        A,(0x321c)       
     LD        (0x2881),A       ; *(0x2881) = *(0x321c);  kod operacji = parametr ze stosu
     LD        HL,0x3223     
     INC       (HL  )   	; *(3223)++; zwiększ licznik pętli
     JP        0x0b56        
; wyjście z pętli wykonania komendy
L0B89:
     LD        A,(0x3223)    
     CP        0x0a         
     JP        NZ,0x0b9b     	; if( *(0x3223) == 0x0a ){ /* jeśli wykonanie komendy się nie udało */
     CALL      0x01f8        	; motor_off();  wyłączenie silników napędów dyskowych
     LD        HL,0x3208     	
     LD        (HL   ),0x02  	;  *(0x3208) = 0x02
     XOR       A           	;  A = 0; return;          /* Wyjdź z funkcji */
     RET                   	;}
; komenda została wykonana 
L0B9B:
     LD        HL,(0x3221)   
     INC       HL        
     LD        (0x3221),HL   	; *(0x3221)++;   następny sektor
     LD        DE,0x0200     
     LD        HL,(0x2885)   
     ADD       HL,DE     	; 
     LD        (0x2885),HL   	; *(0x2885) += 0x200; przesuwa wskaźnik bufora danych na kolejne 512b
     LD        HL,0x3224     
     INC       (HL  )   	; *(0x3224)++
     JP        NZ,0x0b07     	; if( *(0x3224) != 0 ) goto 0xb07;  następny sektor
L0BB3:
     CALL      0x01f8           ; wyłącz silniki FDD
     LD        A,0xff           ; A=0xff; kod błędu
     RET                   	; return

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;


 bb9 LD        A,(0x29ed)    
 bbc INC       A           
 bbd JP        NZ,0x0bc7     
 bc0 LD        HL,0x3208     
 bc3 LD        (HL   ),0x04  
 bc5 XOR       A           
 bc6 RET                   


 bc7 LD        A,(0x29ee)    
 bca CP        0x3a         
 bcc JP        NZ,0x0bec     
 bcf LD        A,(0x29ed)    
 bd2 AND       0xdf         
 bd4 SUB       A,0x41       
 bd6 LD        (0x3209),A    
 bd9 LD        HL,0x29ef     
 bdc LD        DE,0x29ed     
 bdf LD        BC,0x0008     
 be2 LDIR                  
 be4 LD        HL,0x29f5     
 be7 LD        (HL   ),0x20  
 be9 INC       HL        
 bea LD        (HL   ),0x20  


 bec LD        HL,0x3209     
 bef LD        C,(HL  ) 
 bf0 CALL      0x01ed        
 bf3 LD        HL,0x4e20     
 bf6 DEC       HL        
 bf7 LD        A,H         
 bf8 OR        L           
 bf9 JP        NZ,0x0bf6     
 bfc LD        HL,0x3225     
 bff LD        (HL   ),0x00  
 c01 LD        HL,(0x3225)   
 c04 LD        H,0x00       
 c06 LD        BC,0x29ed     
 c09 ADD       HL,BC     
 c0a LD        A,(HL  ) 


 c0b CP        0x20         
 c0d JP        Z,0x0c36      
 c10 LD        A,(0x3225)    
 c13 CP        0x08         
 c15 JP        NC,0x0c36     
 c18 LD        HL,(0x3225)   
 c1b LD        H,0x00       
 c1d LD        BC,0x29ed     
 c20 ADD       HL,BC     
 c21 LD        A,(HL  ) 
 c22 CP        0x61         
 c24 JP        C,0x0c2f      
 c27 CP        0x7b         
 c29 JP        NC,0x0c2f     
 c2c AND       0xdf         


 c2e LD        (HL  ),A 
 c2f LD        HL,0x3225     
 c32 INC       (HL  )   
 c33 JP        0x0c01        
 c36 LD        A,0xff       
 c38 RET                   
 c39 LD        BC,0x0a9a     
 c3c LD        HL,(0x29ec)   
 c3f LD        H,0x00       
 c41 LD        D,H         
 c42 LD        E,L         
 c43 ADD       HL,HL       
 c44 ADD       HL,DE     
 c45 ADD       HL,BC     
 c46 LD        DE,0x320a     


 c49 LD        BC,0x0003     
 c4c LDIR                  
 c4e RET                   
 c4f LD        HL,0x288b     
 c52 LD        DE,0x288c     
 c55 LD        BC,0x015d     
 c58 LD        (HL   ),0x00  
 c5a LDIR                  
 c5c LD        HL,0xffff     
 c5f LD        (0x2887),HL   
 c62 LD        (0x2889),HL   
 c65 XOR       A           
 c66 LD        (0x3226),A    
 c69 LD        HL,0x3228     
 c6c LD        (HL   ),0x00  


 c6e LD        A,(0x3228)    
 c71 CP        0x08         
 c73 JP        NC,0x0ca6     
 c76 LD        A,(0x3226)    
 c79 CALL      0x0aa6        
 c7c PUSH      HL        
 c7d LD        HL,(0x3228)   
 c80 LD        H,0x00       
 c82 LD        BC,0x0010     
 c85 ADD       HL,HL       
 c86 ADD       HL,BC     
 c87 POP       BC          
 c88 ADD       HL,BC     
 c89 LD        C,(HL  ) 
 c8a INC       HL        


 c8b LD        B,(HL  ) 
 c8c LD        H,B         
 c8d LD        L,C         
 c8e LD        (0x3229),HL   
 c91 LD        A,H         
 c92 OR        L           
 c93 JP        Z,0x0c9f      
 c96 LD        HL,(0x3229)   
 c99 LD        BC,0x2887     
 c9c ADD       HL,BC     
 c9d LD        (HL   ),0xff  
 c9f LD        HL,0x3228     
 ca2 INC       (HL  )   
 ca3 JP        NZ,0x0c6e     
 ca6 LD        A,(0x3226)    


 ca9 OR        A           
 caa JP        Z,0x0cc7      
 cad AND       0x3f         
 caf JP        NZ,0x0cc7     
 cb2 LD        C,0x04       
 cb4 PUSH      BC          
 cb5 LD        A,(0x3226)    
 cb8 CALL      0x0abc        
 cbb LD        DE,0x2080     
 cbe CALL      0x0acd        
 cc1 RRA                   
 cc2 JP        C,0x0cc7      
 cc5 XOR       A           
 cc6 RET                   
 cc7 LD        HL,0x3226     


 cca INC       (HL  )   
 ccb JP        NZ,0x0c69     
 cce LD        A,0xff       
 cd0 RET                   
 cd1 LD        HL,0x322b     
 cd4 LD        (HL  ),C 
 cd5 LD        HL,0x322e     
 cd8 LD        (HL   ),0xff  
 cda LD        A,(0x322b)    
 cdd LD        (0x322c),A    
 ce0 OR        A           
 ce1 JP        Z,0x0ce9      
 ce4 AND       0x3f         
 ce6 JP        Z,0x0d91      
 ce9 LD        A,(0x322c)    


 cec CALL      0x0aa6        
 cef LD        A,(HL  ) 
 cf0 CP        0xe5         
 cf2 JP        NZ,0x0cff     
 cf5 LD        HL,0x322c     
 cf8 INC       (HL  )   
 cf9 JP        Z,0x0ddd      
 cfc JP        0x0d8b        
 cff LD        HL,0x322d     
 d02 LD        (HL   ),0x00  
 d04 LD        A,(0x322c)    
 d07 CALL      0x0aa6        
 d0a PUSH      HL        
 d0b LD        HL,(0x322d)   
 d0e LD        H,0x00       


 d10 LD        BC,0x0009     
 d13 ADD       HL,BC     
 d14 POP       BC          
 d15 ADD       HL,BC     
 d16 LD        A,0x7f       
 d18 AND       (HL  )   
 d19 LD        HL,(0x322d)   
 d1c LD        H,0x00       
 d1e LD        BC,0x320a     
 d21 ADD       HL,BC     
 d22 CP        (HL  )   
 d23 JP        NZ,0x0d35     
 d26 LD        A,(0x322d)    
 d29 CP        0x03         
 d2b JP        NC,0x0d35     


 d2e LD        HL,0x322d     
 d31 INC       (HL  )   
 d32 JP        0x0d04        
 d35 LD        A,(0x322d)    
 d38 CP        0x03         
 d3a JP        C,0x0d84      
 d3d LD        HL,0x322d     
 d40 LD        (HL   ),0x00  
 d42 LD        A,(0x322c)    
 d45 CALL      0x0aa6        
 d48 PUSH      HL        
 d49 LD        HL,(0x322d)   
 d4c LD        H,0x00       
 d4e LD        BC,0x0001     
 d51 ADD       HL,BC     


 d52 POP       BC          
 d53 ADD       HL,BC     
 d54 PUSH      HL        
 d55 LD        HL,(0x322d)   
 d58 LD        H,0x00       
 d5a LD        BC,0x29ed     
 d5d ADD       HL,BC     
 d5e POP       BC          
 d5f LD        A,(BC)      
 d60 CP        (HL  )   
 d61 JP        NZ,0x0d73     
 d64 LD        A,(0x322d)    
 d67 CP        0x08         
 d69 JP        NC,0x0d73     
 d6c LD        HL,0x322d     


 d6f INC       (HL  )   
 d70 JP        0x0d42        
 d73 LD        A,(0x322d)    
 d76 CP        0x08         
 d78 JP        C,0x0d84      
 d7b LD        A,(0x322c)    
 d7e LD        (0x320f),A    
 d81 LD        A,0xff       
 d83 RET                   
 d84 LD        HL,0x322c     
 d87 INC       (HL  )   
 d88 JP        Z,0x0ddd      
 d8b LD        A,0x3f       
 d8d AND       (HL  )   
 d8e JP        NZ,0x0dda     


 d91 LD        A,(0x322b)    
 d94 OR        A           
 d95 JP        Z,0x0dbb      
 d98 LD        A,(0x3210)    
 d9b INC       A           
 d9c JP        Z,0x0dbb      
 d9f LD        A,(0x322e)    
 da2 RRA                   
 da3 JP        NC,0x0dbb     
 da6 LD        C,0x06       
 da8 PUSH      BC          
 da9 LD        A,(0x320f)    
 dac CALL      0x0abc        
 daf LD        DE,0x2080     
 db2 CALL      0x0acd        


 db5 RRA                   
 db6 JP        C,0x0dbb      
 db9 XOR       A           
 dba RET                   
 dbb LD        HL,0x3210     
 dbe LD        (HL   ),0x00  
 dc0 LD        HL,0x322e     
 dc3 LD        (HL   ),0x00  
 dc5 LD        C,0x04       
 dc7 PUSH      BC          
 dc8 LD        A,(0x322c)    
 dcb CALL      0x0abc        
 dce LD        DE,0x2080     
 dd1 CALL      0x0acd        
 dd4 RRA                   


 dd5 JP        C,0x0dda      
 dd8 XOR       A           
 dd9 RET                   
 dda JP        0x0ce9        
 ddd LD        HL,0x3208     
 de0 LD        (HL   ),0x04  
 de2 XOR       A           
 de3 RET                   
 de4 LD        A,(0x3203)    
 de7 OR        A           
 de8 JP        Z,0x0e06      
 deb LD        A,(0x3203)    
 dee DEC       A           
 def LD        C,A         
 df0 LD        B,0x00       


 df2 LD        HL,0x3204     
 df5 ADD       HL,BC     
 df6 LD        A,(HL  ) 
 df7 INC       A           
 df8 LD        (0x322f),A    
 dfb JP        Z,0x0eb0      
 dfe AND       0x3f         
 e00 JP        Z,0x0e2a      
 e03 JP        0x0e09        
 e06 LD        (0x322f),A    
 e09 LD        A,(0x322f)    
 e0c CALL      0x0aa6        
 e0f LD        A,(HL  ) 
 e10 CP        0xe5         
 e12 JP        Z,0x0e60      


 e15 LD        HL,0x322f     
 e18 INC       (HL  )   
 e19 JP        Z,0x0e60      
 e1c LD        A,0x3f       
 e1e AND       (HL  )   
 e1f OR        A           
 e20 JP        NZ,0x0e5d     
 e23 LD        A,(0x3203)    
 e26 OR        A           
 e27 JP        Z,0x0e48      
 e2a LD        C,0x06       
 e2c PUSH      BC          
 e2d LD        A,(0x3203)    
 e30 DEC       A           
 e31 LD        C,A         


 e32 LD        B,0x00       
 e34 LD        HL,0x3204     
 e37 ADD       HL,BC     
 e38 LD        A,(HL  ) 
 e39 CALL      0x0abc        
 e3c LD        DE,0x2080     
 e3f CALL      0x0acd        
 e42 RRA                   
 e43 JP        C,0x0e48      
 e46 XOR       A           
 e47 RET                   
 e48 LD        C,0x04       
 e4a PUSH      BC          
 e4b LD        A,(0x322f)    
 e4e CALL      0x0abc        


 e51 LD        DE,0x2080     
 e54 CALL      0x0acd        
 e57 RRA                   
 e58 JP        C,0x0e5d      
 e5b XOR       A           
 e5c RET                   
 e5d JP        0x0e09        
 e60 LD        A,(0x322f)    
 e63 OR        A           
 e64 JP        NZ,0x0e72     
 e67 LD        A,(0x2080)    
 e6a CP        0xe5         
 e6c JP        Z,0x0e72      
 e6f JP        0x0eb0        
 e72 LD        HL,(0x3203)   


 e75 LD        H,0x00       
 e77 LD        BC,0x3204     
 e7a ADD       HL,BC     
 e7b LD        A,(0x322f)    
 e7e LD        (HL  ),A 
 e7f LD        A,(0x322f)    
 e82 CALL      0x0aa6        
 e85 LD        (HL   ),0x00  
 e87 INC       HL        
 e88 EX        DE,HL       
 e89 LD        HL,0x29ed     
 e8c LD        BC,0x0008     
 e8f LDIR                  
 e91 EX        DE,HL       
 e92 CALL      0x0ac4        


 e95 INC       HL        
 e96 LD        A,(0x3203)    
 e99 LD        (HL  ),A 
 e9a INC       HL        
 e9b LD        (HL   ),0x00  
 e9d LD        D,H         
 e9e LD        E,L         
 e9f INC       DE          
 ea0 LD        BC,0x0012     
 ea3 LDIR                  
 ea5 LD        HL,0x3208     
 ea8 LD        (HL   ),0x00  
 eaa LD        A,0xff       
 eac RET                   
 ead JP        0x0eb7        


 eb0 LD        HL,0x3208     
 eb3 LD        (HL   ),0x08  
 eb5 XOR       A           
 eb6 RET                   
 eb7 RET                   
 eb8 LD        A,(0x3208)    
 ebb LD        (0x3231),A    
 ebe LD        HL,0x320a     
 ec1 CALL      0x0ac4        
 ec4 LD        HL,0x320f     
 ec7 LD        (HL   ),0x00  
 ec9 LD        HL,(0x320f)   
 ecc LD        C,L         
 ecd CALL      0x0cd1        
 ed0 RRA                   


 ed1 JP        NC,0x0ee3     
 ed4 LD        A,(0x320f)    
 ed7 CALL      0x0aa6        
 eda LD        (HL   ),0xe5  
 edc LD        HL,0x320f     
 edf INC       (HL  )   
 ee0 JP        0x0ec9        
 ee3 CP        0xc1         
 ee5 JP        C,0x0ef6      
 ee8 LD        C,0x06       
 eea PUSH      BC          
 eeb LD        BC,0x0003     
 eee LD        DE,0x2080     
 ef1 CALL      0x0acd        
 ef4 RRA                   


 ef5 RET       NC          
 ef6 LD        A,(0x3231)    
 ef9 LD        (0x3208),A    
 efc RET                   
 efd LD        HL,0x3233     
 f00 LD        (HL  ),B 
 f01 DEC       HL        
 f02 LD        (HL  ),C 
 f03 LD        A,(0x29ea)    
 f06 CP        0x08         
 f08 JP        NZ,0x0f24     
 f0b LD        HL,0x320f     
 f0e INC       (HL  )   
 f0f INC       HL        
 f10 LD        (HL   ),0xff  


 f12 LD        HL,(0x320f)   
 f15 LD        C,L         
 f16 CALL      0x0cd1        
 f19 RRA                   
 f1a JP        C,0x0f1f      
 f1d XOR       A           
 f1e RET                   
 f1f LD        HL,0x29ea     
 f22 LD        (HL   ),0x00  
 f24 LD        A,(0x320f)    
 f27 CALL      0x0aa6        
 f2a LD        BC,0x0010     
 f2d ADD       HL,BC     
 f2e PUSH      HL        
 f2f LD        HL,(0x29ea)   


 f32 LD        H,0x00       
 f34 ADD       HL,HL       
 f35 POP       BC          
 f36 ADD       HL,BC     
 f37 LD        E,(HL  ) 
 f38 INC       HL        
 f39 LD        D,(HL  ) 
 f3a EX        DE,HL       
 f3b LD        (0x3236),HL   
 f3e LD        A,H         
 f3f OR        L           
 f40 JP        Z,0x0f5f      
 f43 LD        C,0x04       
 f45 PUSH      BC          
 f46 LD        B,H         


 f47 LD        C,L         
 f48 LD        HL,(0x3232)   
 f4b EX        DE,HL       
 f4c CALL      0x0acd        
 f4f RRA                   
 f50 JP        C,0x0f55      
 f53 XOR       A           
 f54 RET                   
 f55 LD        HL,0x29ea     
 f58 INC       (HL  )   
 f59 LD        A,0xff       
 f5b RET                   
 f5c JP        0x0f66        
 f5f LD        HL,0x3208     
 f62 LD        (HL   ),0x04  


 f64 XOR       A           
 f65 RET                   
 f66 LD        HL,0x3235     
 f69 LD        (HL  ),B 
 f6a DEC       HL        
 f6b LD        (HL  ),C 
 f6c XOR       A           
 f6d LD        (0x3210),A    
 f70 LD        HL,0x0004     
 f73 LD        (0x3238),HL   
 f76 LD        HL,(0x3238)   
 f79 LD        BC,0x2887     
 f7c ADD       HL,BC     
 f7d LD        A,(HL  ) 
 f7e OR        A           


 f7f JP        Z,0x0f9b      
 f82 LD        HL,(0x3238)   
 f85 INC       HL        
 f86 LD        (0x3238),HL   
 f89 LD        DE,0x0163     
 f8c SBC       HL,DE     
 f8e JP        C,0x0f76      
 f91 LD        HL,0x3208     
 f94 LD        (HL   ),0x08  
 f96 XOR       A           
 f97 RET                   
 f98 JP        0x0f76        
 f9b LD        A,(0x29ea)    
 f9e CP        0x08         
 fa0 JP        NZ,0x0fc6     


 fa3 LD        A,(0x323a)    
 fa6 CALL      0x0aa6        
 fa9 LD        BC,0x3203     
 fac ADD       HL,BC     
 fad LD        A,(0x3203)    
 fb0 LD        (HL  ),A 
 fb1 LD        HL,0x3203     
 fb4 INC       (HL  )   
 fb5 CALL      0x0de4        
 fb8 RRA                   
 fb9 JP        C,0x0fbe      
 fbc XOR       A           
 fbd RET                   
 fbe LD        HL,0x29ea     
 fc1 LD        (HL   ),0x00  


 fc3 INC       HL        
 fc4 LD        (HL   ),0x00  
 fc6 LD        HL,(0x3238)   
 fc9 LD        (0x3236),HL   
 fcc LD        C,0x06       
 fce PUSH      BC          
 fcf LD        B,H         
 fd0 LD        C,L         
 fd1 LD        DE,0x29fd     
 fd4 CALL      0x0acd        
 fd7 RRA                   
 fd8 JP        C,0x0fdd      
 fdb XOR       A           
 fdc RET                   
 fdd LD        HL,(0x3238)   


 fe0 LD        BC,0x2887     
 fe3 ADD       HL,BC     
 fe4 LD        (HL   ),0xff  
 fe6 LD        HL,(0x3203)   
 fe9 LD        H,0x00       
 feb LD        BC,0x3204     
 fee ADD       HL,BC     
 fef LD        A,(HL  ) 
 ff0 LD        (0x323a),A    
 ff3 CALL      0x0aa6        
 ff6 PUSH      HL        
 ff7 LD        HL,(0x29ea)   
 ffa LD        H,0x00       
 ffc LD        BC,0x0010     
 fff ADD       HL,HL       


1000 ADD       HL,BC     
1001 POP       BC          
1002 ADD       HL,BC     
1003 PUSH      HL        
1004 LD        HL,(0x3236)   
1007 EX        DE,HL       
1008 POP       HL        
1009 LD        (HL  ),E 
100a INC       HL        
100b LD        (HL  ),D 
100c LD        HL,(0x3234)   
100f LD        A,L         
1010 RLA                   
1011 LD        A,H         
1012 RLA                   


1013 LD        E,A         
1014 LD        A,0x00       
1016 RLA                   
1017 LD        D,A         
1018 LD        HL,0x29eb     
101b LD        A,(HL  ) 
101c ADD       A,E         
101d LD        (HL  ),A 
101e LD        A,(0x3234)    
1021 AND       0x7f         
1023 JP        Z,0x1027      
1026 INC       (HL  )   
1027 LD        A,(0x323a)    
102a CALL      0x0aa6        
102d LD        BC,0x000f     


1030 ADD       HL,BC     
1031 LD        A,(0x29eb)    
1034 LD        (HL  ),A 
1035 LD        HL,0x29ea     
1038 INC       (HL  )   
1039 LD        A,0xff       
103b RET                   
103c LD        HL,0x3215     
103f LD        (HL  ),D 
1040 DEC       HL        
1041 LD        (HL  ),E 
1042 DEC       HL        
1043 LD        (HL  ),B 
1044 DEC       HL        
1045 LD        (HL  ),C 


1046 DEC       HL        
1047 POP       DE          
1048 POP       BC          
1049 LD        (HL  ),C 
104a PUSH      DE          
104b XOR       A           
104c LD        (0x3210),A    
104f LD        A,(0x3211)    
1052 OR        A           
1053 JP        NZ,0x10be     
1056 LD        HL,(0x3212)   
1059 LD        DE,0x29ec     
105c LD        BC,0x0011     
105f LDIR                  
1061 CALL      0x0bb9        


1064 RRA                   
1065 RET       NC          
1066 LD        C,0x04       
1068 PUSH      BC          
1069 LD        DE,0x2080     
106c LD        BC,0x0000     
106f CALL      0x0acd        
1072 RRA                   
1073 RET       NC          
1074 LD        HL,0x3203     
1077 LD        (HL   ),0x00  
1079 CALL      0x0de4        
107c RRA                   
107d RET       NC          
107e LD        C,0x06       


1080 PUSH      BC          
1081 LD        HL,(0x3203)   
1084 LD        H,0x00       
1086 LD        BC,0x3204     
1089 ADD       HL,BC     
108a LD        A,(HL  ) 
108b CALL      0x0abc        
108e LD        DE,0x2080     
1091 CALL      0x0acd        
1094 RRA                   
1095 RET       NC          
1096 LD        HL,(0x3203)   
1099 LD        H,0x00       
109b LD        BC,0x3204     
109e ADD       HL,BC     


109f LD        A,(HL  ) 
10a0 AND       0xc0         
10a2 JP        Z,0x10b3      
10a5 LD        C,0x04       
10a7 PUSH      BC          
10a8 LD        DE,0x2080     
10ab LD        BC,0x0000     
10ae CALL      0x0acd        
10b1 RRA                   
10b2 RET       NC          
10b3 CALL      0x0c4f        
10b6 RRA                   
10b7 RET       NC          
10b8 LD        HL,0x3208     
10bb LD        (HL   ),0x00  


10bd RET                   
10be LD        HL,0x29ec     
10c1 LD        DE,0x29fd     
10c4 LD        BC,0x0011     
10c7 LDIR                  
10c9 LD        DE,0x0011     
10cc LD        HL,(0x3214)   
10cf ADD       HL,DE     
10d0 LD        (0x31ff),HL   
10d3 LD        HL,(0x3212)   
10d6 LD        (0x320d),HL   
10d9 LD        HL,0x07ef     
10dc LD        (0x3201),HL   
10df LD        HL,0x2a0e     
10e2 LD        (0x31fd),HL   


10e5 LD        HL,0x29eb     
10e8 LD        (HL   ),0x00  
10ea DEC       HL        
10eb LD        (HL   ),0x00  
10ed LD        C,0x04       
10ef PUSH      BC          
10f0 LD        A,(0x3204)    
10f3 CALL      0x0abc        
10f6 LD        DE,0x2080     
10f9 CALL      0x0acd        
10fc RRA                   
10fd RET       NC          
10fe LD        HL,(0x31ff)   
1101 LD        DE,0x0801     
1104 OR        A           


1105 SBC       HL,DE     
1107 JP        C,0x1169      
110a LD        HL,(0x3201)   
110d LD        B,H         
110e LD        C,L         
110f LD        HL,(0x31fd)   
1112 EX        DE,HL       
1113 LD        HL,(0x320d)   
1116 LDIR                  
1118 LD        BC,0x0800     
111b CALL      0x0f66        
111e RRA                   
111f JP        C,0x1143      
1122 LD        HL,(0x3203)   
1125 LD        H,0x00       


1127 LD        BC,0x3204     
112a ADD       HL,BC     
112b LD        A,(HL  ) 
112c AND       0xc0         
112e JP        Z,0x113f      
1131 LD        C,0x04       
1133 PUSH      BC          
1134 LD        DE,0x2080     
1137 LD        BC,0x0000     
113a CALL      0x0acd        
113d RRA                   
113e RET       NC          
113f CALL      0x0eb8        
1142 RET                   
1143 LD        HL,(0x31ff)   


1146 LD        DE,0x0800     
1149 OR        A           
114a SBC       HL,DE     
114c LD        (0x31ff),HL   
114f LD        HL,(0x3201)   
1152 EX        DE,HL       
1153 LD        HL,(0x320d)   
1156 ADD       HL,DE     
1157 LD        (0x320d),HL   
115a LD        HL,0x0800     
115d LD        (0x3201),HL   
1160 LD        HL,0x29fd     
1163 LD        (0x31fd),HL   
1166 JP        0x10fe        
1169 LD        HL,(0x31ff)   


116c LD        A,H         
116d OR        L           
116e JP        Z,0x11ac      
1171 LD        HL,(0x3201)   
1174 LD        B,H         
1175 LD        C,L         
1176 LD        HL,(0x31fd)   
1179 EX        DE,HL       
117a LD        HL,(0x320d)   
117d LDIR                  
117f LD        HL,(0x31ff)   
1182 LD        B,H         
1183 LD        C,L         
1184 CALL      0x0f66        
1187 RRA                   


1188 JP        C,0x11ac      
118b LD        HL,(0x3203)   
118e LD        H,0x00       
1190 LD        BC,0x3204     
1193 ADD       HL,BC     
1194 LD        A,(HL  ) 
1195 AND       0xc0         
1197 JP        Z,0x11a8      
119a LD        C,0x04       
119c PUSH      BC          
119d LD        DE,0x2080     
11a0 LD        BC,0x0000     
11a3 CALL      0x0acd        
11a6 RRA                   
11a7 RET       NC          


11a8 CALL      0x0eb8        
11ab RET                   
11ac LD        C,0x06       
11ae PUSH      BC          
11af LD        HL,(0x3203)   
11b2 LD        H,0x00       
11b4 LD        BC,0x3204     
11b7 ADD       HL,BC     
11b8 LD        A,(HL  ) 
11b9 CALL      0x0abc        
11bc LD        DE,0x2080     
11bf CALL      0x0acd        
11c2 RRA                   
11c3 RET       NC          
11c4 LD        HL,(0x3203)   


11c7 LD        H,0x00       
11c9 LD        BC,0x3204     
11cc ADD       HL,BC     
11cd LD        A,(HL  ) 
11ce AND       0xc0         
11d0 JP        Z,0x11e1      
11d3 LD        C,0x04       
11d5 PUSH      BC          
11d6 LD        DE,0x2080     
11d9 LD        BC,0x0000     
11dc CALL      0x0acd        
11df RRA                   
11e0 RET       NC          
11e1 CALL      0x0c39        
11e4 LD        HL,0x320f     


11e7 LD        (HL   ),0x00  
11e9 LD        HL,(0x320f)   
11ec LD        C,L         
11ed CALL      0x0cd1        
11f0 RRA                   
11f1 JP        NC,0x1203     
11f4 LD        A,(0x320f)    
11f7 CALL      0x0aa6        
11fa LD        (HL   ),0xe5  
11fc LD        HL,0x320f     
11ff INC       (HL  )   
1200 JP        0x11e9        
1203 LD        A,(0x320f)    
1206 CP        0xc1         
1208 JP        C,0x1219      


120b LD        C,0x06       
120d PUSH      BC          
120e LD        BC,0x0003     
1211 LD        DE,0x2080     
1214 CALL      0x0acd        
1217 RRA                   
1218 RET       NC          
1219 LD        HL,0x320f     
121c LD        (HL   ),0xc0  
121e LD        HL,0x322e     
1221 LD        (HL   ),0x00  
1223 LD        HL,0x3216     
1226 LD        (HL   ),0x00  
1228 LD        A,(0x3203)    
122b LD        HL,0x3216     


122e CP        (HL  )   
122f JP        C,0x1296      
1232 LD        HL,(0x3216)   
1235 LD        H,0x00       
1237 LD        BC,0x3204     
123a ADD       HL,BC     
123b LD        A,(HL  ) 
123c LD        (0x3217),A    
123f AND       0xc0         
1241 LD        B,A         
1242 LD        HL,0x320f     
1245 LD        A,0xc0       
1247 AND       (HL  )   
1248 CP        B           
1249 JP        Z,0x1276      


124c LD        A,(0x322e)    
124f RRA                   
1250 JP        NC,0x1262     
1253 LD        C,0x06       
1255 PUSH      BC          
1256 LD        A,(HL  ) 
1257 CALL      0x0abc        
125a LD        DE,0x2080     
125d CALL      0x0acd        
1260 RRA                   
1261 RET       NC          
1262 LD        HL,0x322e     
1265 LD        C,0x04       
1267 PUSH      BC          
1268 LD        A,(0x3217)    


126b CALL      0x0abc        
126e LD        DE,0x2080     
1271 CALL      0x0acd        
1274 RRA                   
1275 RET       NC          
1276 LD        A,(0x3217)    
1279 CALL      0x0aa6        
127c LD        BC,0x0009     
127f ADD       HL,BC     
1280 EX        DE,HL       
1281 LD        BC,0x0003     
1284 LD        HL,0x320a     
1287 LDIR                  
1289 LD        A,(0x3217)    
128c LD        (0x320f),A    


128f LD        HL,0x3216     
1292 INC       (HL  )   
1293 JP        NZ,0x1228     
1296 LD        C,0x06       
1298 PUSH      BC          
1299 LD        A,(0x3217)    
129c CALL      0x0abc        
129f LD        DE,0x2080     
12a2 CALL      0x0acd        
12a5 RRA                   
12a6 RET       NC          
12a7 LD        HL,0x3208     
12aa LD        (HL   ),0x00  
12ac RET                   
12ad LD        HL,0x3215     


12b0 LD        (HL  ),D 
12b1 DEC       HL        
12b2 LD        (HL  ),E 
12b3 DEC       HL        
12b4 LD        (HL  ),B 
12b5 DEC       HL        
12b6 LD        (HL  ),C 
12b7 DEC       HL        
12b8 POP       DE          
12b9 POP       BC          
12ba LD        (HL  ),C 
12bb PUSH      DE          
12bc LD        A,(0x3211)    
12bf OR        A           
12c0 JP        NZ,0x1335     


12c3 LD        BC,0x0011     
12c6 LD        HL,(0x3212)   
12c9 LD        D,B         
12ca LD        E,C         
12cb OR        A           
12cc SBC       HL,DE     
12ce LD        DE,0x29ec     
12d1 LDIR                  
12d3 CALL      0x0bb9        
12d6 RRA                   
12d7 RET       NC          
12d8 LD        C,0x04       
12da PUSH      BC          
12db LD        DE,0x2080     
12de LD        BC,0x0000     


12e1 CALL      0x0acd        
12e4 RRA                   
12e5 RET       NC          
12e6 CALL      0x0c39        
12e9 LD        HL,0x320f     
12ec LD        (HL   ),0x00  
12ee LD        HL,(0x320f)   
12f1 LD        C,L         
12f2 CALL      0x0cd1        
12f5 RRA                   
12f6 RET       NC          
12f7 LD        C,0x04       
12f9 PUSH      BC          
12fa LD        A,(0x320f)    
12fd CALL      0x0aa6        


1300 LD        BC,0x0010     
1303 ADD       HL,BC     
1304 LD        C,(HL  ) 
1305 INC       HL        
1306 LD        B,(HL  ) 
1307 LD        DE,0x29fd     
130a CALL      0x0acd        
130d RRA                   
130e RET       NC          
130f LD        HL,(0x3212)   
1312 EX        DE,HL       
1313 LD        BC,0x0011     
1316 LD        HL,0x29fd     
1319 LDIR                  
131b LD        HL,(0x3212)   


131e INC       HL        
131f PUSH      HL        
1320 LD        BC,0x000a     
1323 LD        HL,(0x3212)   
1326 LD        DE,0x0010     
1329 OR        A           
132a SBC       HL,DE     
132c POP       DE          
132d LDIR                  
132f LD        HL,0x3208     
1332 LD        (HL   ),0x00  
1334 RET                   
1335 LD        HL,(0x3214)   
1338 LD        (0x31ff),HL   
133b LD        HL,(0x3212)   


133e LD        (0x320d),HL   
1341 LD        HL,0x07ef     
1344 LD        (0x3201),HL   
1347 LD        HL,0x2a0e     
134a LD        (0x31fd),HL   
134d LD        HL,(0x31ff)   
1350 EX        DE,HL       
1351 LD        HL,(0x3201)   
1354 OR        A           
1355 SBC       HL,DE     
1357 JP        NC,0x13d9     
135a LD        HL,(0x3201)   
135d LD        B,H         
135e LD        C,L         
135f LD        HL,(0x320d)   


1362 EX        DE,HL       
1363 LD        HL,(0x31fd)   
1366 LDIR                  
1368 LD        HL,(0x3201)   
136b EX        DE,HL       
136c LD        HL,(0x31ff)   
136f OR        A           
1370 SBC       HL,DE     
1372 LD        (0x31ff),HL   
1375 LD        HL,(0x3201)   
1378 EX        DE,HL       
1379 LD        HL,(0x320d)   
137c ADD       HL,DE     
137d LD        (0x320d),HL   
1380 LD        HL,0x0800     


1383 LD        (0x3201),HL   
1386 LD        HL,0x29ea     
1389 LD        (HL   ),0x01  
138b LD        HL,(0x31ff)   
138e EX        DE,HL       
138f LD        HL,(0x3201)   
1392 OR        A           
1393 SBC       HL,DE     
1395 JP        NC,0x13c3     
1398 LD        HL,(0x320d)   
139b LD        B,H         
139c LD        C,L         
139d CALL      0x0efd        
13a0 RRA                   
13a1 RET       NC          


13a2 LD        HL,(0x3201)   
13a5 EX        DE,HL       
13a6 LD        HL,(0x31ff)   
13a9 OR        A           
13aa SBC       HL,DE     
13ac LD        (0x31ff),HL   
13af LD        HL,(0x3201)   
13b2 EX        DE,HL       
13b3 LD        HL,(0x320d)   
13b6 ADD       HL,DE     
13b7 LD        (0x320d),HL   
13ba LD        HL,0x0800     
13bd LD        (0x3201),HL   
13c0 JP        0x138b        
13c3 LD        HL,(0x31ff)   


13c6 LD        A,H         
13c7 OR        L           
13c8 JP        Z,0x13d9      
13cb LD        BC,0x29fd     
13ce CALL      0x0efd        
13d1 RRA                   
13d2 RET       NC          
13d3 LD        HL,0x29fd     
13d6 LD        (0x31fd),HL   
13d9 LD        HL,(0x31ff)   
13dc LD        A,H         
13dd OR        L           
13de JP        Z,0x13ef      
13e1 LD        HL,(0x31ff)   
13e4 LD        B,H         


13e5 LD        C,L         
13e6 LD        HL,(0x320d)   
13e9 EX        DE,HL       
13ea LD        HL,(0x31fd)   
13ed LDIR                  
13ef LD        HL,0x3208     
13f2 LD        (HL   ),0x00  
13f4 RET                   

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Funkcja obsługi komendy interpretera BASIC DIR [A|B]
;;
;; Wejście:
;;
;; DE - bufor zmiennych operacji
;; C  - numer napędu (0 dla domyślnego,  1 dla 'A', lub 2 dla 'B' ) -> disk
;; (3209) - numer domyślnego napędu,

L13F5:
     LD        HL,0x3219     
     LD        (HL  ),D 	; 
     DEC       HL        
     LD        (HL  ),E 	; (0x3218) = DE
     LD        DE,0x0013     
     LD        HL,(0x3218)      ; 
     ADD       HL,DE     
     LD        (0x321a),HL	; (0x321a) = DE + 0x13; bufor na nazwy plików
     LD        A,C         
     OR        A           
     JP        Z,0x140e    	; if(C != 0x00 ) (0x3209) = disk - 1
     DEC       A           
     LD        (0x3209),A    
L140E:
     LD        HL,(0x3218)      ; HL = DE
     EX        DE,HL       
; utworzenie napisu "\nKatalog dysku C:\n\n", gdzie C - to litera A lub B
     LD        BC,0x000f     
     LD        HL,0x0a8b        ; łańcuch znaków: "\nKatalog dysku "
     LDIR                  	; [ (DE + i) <- (0x0a8b + i) ], i=00..14 
     LD        A,(0x3209)       ; 
     ADD       A,0x41       
     LD        BC,0x000f     
     LD        HL,(0x3218)   
     ADD       HL,BC     
     LD        (HL  ),A 	; (DE + 0x0f) = 'A' + disk
     INC       HL        
     LD        (HL   ),0x3a  	; (DE + 0x10) = ':'
     INC       HL        
     LD        (HL   ),0x0d     ; (DE + 0x11) = '\n'
     INC       HL        
     LD        (HL   ),0x0d     ; (DE + 0x12) = '\n'
     LD        HL,(0x3209)      ; 
     LD        C,L		; C = L = disk
     CALL      0x01ed           ; załączenie silnika napędu 

; opóźnienie
     LD        A,0x0b       
L1439:
     DEC       B           
     JP        NZ,0x1439     
     DEC       A           
     JP        NZ,0x1439     ; pusta pętla opóźnienia

     LD        C,0x04        ; Czytaj kluster z FDD
     PUSH      BC            ; parametr dla funkcji L0ACD
     LD        DE,0x2080     ; bufor danych odczytanych ( 2kB )
     LD        BC,0x0000     ; sektor 1 ścieżka 1
     CALL      0x0acd        ; odczyt klastra
     RRA                   
     JP        C,0x1455      ; if(A == 0xff) goto 1455  /* pomyślny odczyt klustra */
     LD        HL,0x0000     ; HL=00; - błąd
     RET                     ; wyjdź z funkcji

; odczytano pierwszy kluster FDD
L1455:
     LD        HL,0x3217     
     LD        (HL   ),0x00  ; *(0x3217) = 0; licznik pozycji katalogu
     DEC       HL        
     LD        (HL   ),0x00  ; *(0x3216) = 0; nr FCB w katalogu
L145D:
     LD        A,(0x3216)    ; A = *(0x3216);
     CALL      0x0aa6        ; HL = 0x2080 + 32 * (A & 0x3f); /* wskazanie na pozycję katalogu */
; test pól FCB
     LD        A,(HL  )	     ; Właściciel pliku, jeśli #E5 tzn że plik skasowano
     OR        A           
     JP        NZ,0x1494     ; if(*(HL+00) != 0 ) goto 0x1494; /* jeśli pierwsze pole !=0 skocz */
     LD        BC,0x000c     
     ADD       HL,BC     
     LD        A,(HL  )      ; nr tablicy
     OR        A           
     JP        NZ,0x1494     ; if(*(HL+0c) != 0 ) goto 0x1494;     
     LD        A,(0x3216)    ; A = *(0x3216); pozycja w katalogu
     CALL      0x0aa6        ; HL - wskaźnik na pozycję w katalogu
     INC       HL            ; początek nazwy pliku
; obliczenie pozycji w buforze wyniku
     PUSH      HL        
     LD        HL,(0x3217)   ; BYTE PTR HL = *(0x3217)
     LD        H,0x00       
     CALL      0x0ab5        ; HL = HL*12
     EX        DE,HL         ; DE = HL
     LD        HL,(0x321a)   ; HL = *(0x321a); wskaźnik na bufor nazw wyniku do listingu
     ADD       HL,DE
     EX        DE,HL         ; DE = *(0x321a) + *(0x3217) * 12;
     LD        BC,0x000b     ; 11 bajtów do skopiowania
     POP       HL        
     LDIR                    ; *(DE++) <- *(HL++) ; przekopiowanie łańcucha 11 znaków nazwy pliku
     LD        A,0x0d       
     LD        (DE),A        ; *(DE+11) = 0x0d
     LD        HL,0x3217     
     INC       (HL  )        ; *(0x3217)++; licznik pozycji katalogu

; nazwa pliku skopiowana
;ląduje tutaj także gdy właściciel pliku ma ID inny niż 0, lub nr tablicy FCB nie jest 0
L1494:
     LD        HL,0x3216     
     INC       (HL  )   	; *(0x3216)++; licznik tablic FCB++
     LD        A,(HL  )         ; 
     CALL      0x0aa6           ; HL <- adres kolejnej tablicy FCB
     AND       0x3f         	; 
     JP        NZ,0x14b8     	; if(( *(0x13216) & 0x3f) != 0) goto 14B8;
     LD        C,0x04       
     PUSH      BC          	; kod operacji: READ CLUSTER
     LD        A,(0x3216)       ; numer klastra do odczytu
     CALL      0x0abc        	; BC = 4*A; przeliczenie numeru klastra na sektor
     LD        DE,0x2080     	; bufor danych (2kB)
     CALL      0x0acd        	; odczyt klastra
     RRA                   
     JP        C,0x14b8         ; if(A == 0xff) goto L14B8
     LD        HL,0x0000        ; HL <- kod błędu
     RET                        ; wyjdź
L14B8:
     LD        A,(0x3216)    
     OR        A           
     JP        NZ,0x145d        ; if(*(3216) != 0) goto L145d ;odczyt kolejnej pozycji katalogu

; koniec odczytu katalogu, wszystkie pozycje zostały odczytane
     LD        HL,(0x3217)   	
     LD        H,0x00       
     CALL      0x0ab5
     EX        DE,HL           	; DE = *(0x3217) * 12
     LD        HL,(0x321a)   
     ADD       HL,DE     
     LD        (HL   ),0x0d     ; *( *(0x3217) * 12 + *(0x321a)) = 0x0d; wstawienie '\n' na końcu listy
     LD        HL,0x3208     
     LD        (HL   ),0x00     ; *(0x3208) = 0
     LD        HL,(0x3217)   
     LD        H,0x00       
     CALL      0x0ab5           ; HL = *(0x3217) * 12;
     LD        BC,0x0013     
     ADD       HL,BC     	
     INC       HL        	; HL *(0x3217) * 12 + 0x13 + 1
     RET                        ; koniec funkcji /* 14e0 */




14e1 LD        HL,0x3215     
14e4 LD        (HL  ),D 
14e5 DEC       HL        
14e6 LD        (HL  ),E 
14e7 DEC       HL        
14e8 LD        (HL  ),B 
14e9 DEC       HL        
14ea LD        (HL  ),C 
14eb DEC       HL        
14ec POP       DE          
14ed POP       BC          
14ee LD        (HL  ),C 
14ef PUSH      DE          


14f0 XOR       A           
14f1 LD        (0x3210),A    
14f4 LD        HL,(0x3212)   
14f7 LD        DE,0x29ec     
14fa LD        BC,0x0011     
14fd LDIR                  
14ff CALL      0x0bb9        
1502 RRA                   
1503 RET       NC          
1504 LD        C,0x04       
1506 PUSH      BC          
1507 LD        DE,0x2080     
150a LD        BC,0x0000     
150d CALL      0x0acd        
1510 RRA                   


1511 RET       NC          
1512 CALL      0x0c39        
1515 LD        HL,0x320f     
1518 LD        (HL   ),0x00  
151a LD        HL,(0x320f)   
151d LD        C,L         
151e CALL      0x0cd1        
1521 RRA                   
1522 RET       NC          
1523 LD        A,(0x320f)    
1526 CALL      0x0aa6        
1529 LD        (HL   ),0xe5  
152b LD        HL,0x320f     
152e INC       (HL  )   
152f LD        HL,(0x320f)   


1532 LD        C,L         
1533 CALL      0x0cd1        
1536 RRA                   
1537 JP        NC,0x1549     
153a LD        A,(0x320f)    
153d CALL      0x0aa6        
1540 LD        (HL   ),0xe5  
1542 LD        HL,0x320f     
1545 INC       (HL  )   
1546 JP        0x152f        
1549 LD        A,(0x320f)    
154c CP        0xc1         
154e JP        C,0x155f      
1551 LD        C,0x06       
1553 PUSH      BC          


1554 LD        BC,0x0003     
1557 LD        DE,0x2080     
155a CALL      0x0acd        
155d RRA                   
155e RET       NC          
155f LD        HL,0x3208     
1562 LD        (HL   ),0x00  
1564 RET                   
1565 EX        DE,HL       

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Działanie: 
;;
;;   Funkcja realizuje obliczanie adresu elementu tablicy jednowymiarowej:
;;   	HL = &tablica[A]
;;	adres zmiennej 'tablica' jest podany w HL
;;
L1566:

     LD        E,A         
     LD        D,0x00       
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Działanie: 
;;
;;   Funkcja realizuje obliczanie adresu elementu tablicy jednowymiarowej:
;;   HL = &tablica[DE]
;;   
;;  Szczegóły działania:
;; 	HL = DE + (HL)
;; 	DE = HL + 1
;; 	A  = H (wyniku)
;; 

L1569:
     EX        DE,HL        
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Działanie: 
;;
;;   Funkcja realizuje obliczanie adresu elementu tablicy jednowymiarowej:
;;   HL = &tablica[HL] - tablica = DE
;;   
;;  Szczegóły działania:
;; 	HL = HL + (DE)
;; 	DE = DE + 1
;; 	A  = H (wyniku)
;; 

L156A:
     LD        A,(DE)       
     ADD       A,L          
     LD        L,A          
     INC       DE           
     LD        A,(DE)       
     ADC       A,H          
     LD        H,A 
     RET                   

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Pętla opóźniająca
;;
;; Parametry wejściowe 
;; A - czas opóźnienia
;;
;; Modyfikuje AF, BC
;;
L1572:
     LD        B,0x0c       ; B=12
L1574:
     LD        C,B          ; C=12
L1575:
     DEC       C            ; C--
     JP        NZ,0x1575    ; if (C > 0) goto L1575
     DEC       A           
     JP        NZ,0x1574      
     RET                   


157e ADC       A,H         
157f LD        A,(BC)      
1580 RRA                   
1581 JP        C,0x1585      
1584 RET                   
1585 CALL      0x0c16        
1588 LD        HL,0x30f1     
158b LD        (HL   ),0x00  
158d LD        HL,(0x30f1)   
1590 LD        C,L         
1591 CALL      0x0cf2        
1594 RRA                   
1595 JP        C,0x1599      
1598 RET                   

1599 LD        HL,0x30f1     
159c LD        (HL   ),0x00  
159e LD        HL,(0x30f1)   
15a1 LD        C,L         
15a2 CALL      0x0cf2        
15a5 RRA                   
15a6 JP        NC,0x15c1     
15a9 LD        DE,0x0020     
15ac LD        HL,(0x30f1)   
15af LD        H,0x00       
15b1 CALL      0x1603        
15b4 LD        BC,0x2080     
15b7 ADD       HL,BC     
15b8 LD        (HL   ),0xe5  
15ba LD        HL,0x30f1     
15bd INC       (HL  )   
15be JP        0x159e        
15c1 LD        C,0x06       
15c3 PUSH      BC          


15c4 LD        DE,0x2080     
15c7 LD        C,0x00       
15c9 CALL      0x0a8c        
15cc RRA                   
15cd JP        C,0x15d1      
15d0 RET                   
15d1 LD        HL,0x30ea     
15d4 LD        (HL   ),0x00  
15d6 RET                   
15d7 EX        DE,HL       
15d8 LD        E,A         
15d9 LD        D,0x00       
15db EX        DE,HL       
15dc LD        A,(DE)      
15dd ADD       A,L         


15de LD        L,A         
15df INC       DE          
15e0 LD        A,(DE)      
15e1 ADC       A,H         
15e2 LD        H,A         
15e3 RET                   
15e4 LD        B,H         
15e5 LD        C,L         
15e6 LD        HL,0x0000     
15e9 LD        A,0x10       
15eb PUSH      AF          
15ec ADD       HL,HL       
15ed EX        DE,HL       
15ee SUB       A,A         
15ef ADD       HL,HL       


15f0 EX        DE,HL       
15f1 ADC       A,L         
15f2 SUB       A,C         
15f3 LD        L,A         
15f4 LD        A,H         
15f5 SBC       A,B         
15f6 LD        H,A         
15f7 INC       DE          
15f8 JP        NC,0x15fd     
15fb ADD       HL,BC     
15fc DEC       DE          
15fd POP       AF          
15fe DEC       A           
15ff JP        NZ,0x15eb     
1602 RET                   


1603 LD        B,H         
1604 LD        C,L         
1605 LD        HL,0x0000     
1608 LD        A,0x10       
160a ADD       HL,HL       
160b EX        DE,HL       
160c ADD       HL,HL       
160d EX        DE,HL       
160e JP        NC,0x1612     
1611 ADD       HL,BC     
1612 DEC       A           
1613 JP        NZ,0x160a     
1616 RET                   
1617 LD        E,A         
1618 LD        D,0x00       


161a LD        A,E         
161b SUB       A,L         
161c LD        L,A         
161d LD        A,D         
161e SBC       A,H         
161f LD        H,A         
1620 RET                   
1621 LD        C,A         
1622 LD        B,0x00       
1624 LD        A,E         
1625 SUB       A,C         
1626 LD        L,A         
1627 LD        A,D         
1628 SBC       A,B         
1629 LD        H,A         


162a RET                   
162b LD        L,C         
162c LD        H,B         
162d LD        C,(HL  ) 
162e INC       HL        
162f LD        B,(HL  ) 
1630 LD        A,(DE)      
1631 SUB       A,C         
1632 LD        L,A         
1633 INC       DE          
1634 LD        A,(DE)      
1635 SBC       A,B         
1636 LD        H,A         
1637 RET                   
1638 LD        L,A         


1639 LD        H,0x00       
163b LD        A,(DE)      
163c SUB       A,L         
163d LD        L,A         
163e INC       DE          
163f LD        A,(DE)      
1640 SBC       A,H         
1641 LD        H,A         
1642 RET                   
1643 LD        E,A         
1644 LD        D,0x00       
1646 LD        A,E         
1647 SUB       A,(HL  ) 
1648 LD        E,A         
1649 LD        A,D         


164a INC       HL        
164b SBC       A,(HL  ) 
164c LD        D,A         
164d EX        DE,HL       
164e RET                   
164f LD        B,0x0c       
1651 LD        C,B         
1652 DEC       C           
1653 JP        NZ,0x1652     
1656 DEC       A           
1657 JP        NZ,0x1651     
165a RET                   
165b RST       56         
165c RST       56         
165d RST       56         


165e RST       56         
165f RST       56         

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; ......@@@@@@@@.....@@.......@@......@@......@@......@@@@@......@@@@@@@..........
;; ...@@@@@@@@@@@@...@@@@.....@@@@....@@@@....@@@@....@@@@@@@....@@@@@@@@@@........
;; ..@@@@@@@@@@@@@...@@@@.....@@@@....@@@@....@@@@...@@@@@@@@..@@@@@@@@@@@@@.......
;; .@@@@@@@@@@@@@....@@@@.....@@@@....@@@@....@@@@...@@@@@@@..@@@@@@...@@@@@@......
;; .@@@@@............@@@@.....@@@@....@@@@....@@@@...@@@@.....@@@@.......@@@@......
;; @@@@@.............@@@@.....@@@@....@@@@....@@@@...@@@@....@@@@.........@@@@.....
;; @@@@@@@@@@@@@@@...@@@@.....@@@@....@@@@....@@@@...@@@@....@@@@.........@@@@.....
;; ................................................................................
;; @@@@@@@@@@@@@@@...@@@@.....@@@@....@@@@....@@@@...@@@@....@@@@.........@@@@.....
;; @@@@@.............@@@@.....@@@@....@@@@....@@@@...@@@@....@@@@.........@@@@.....
;; .@@@@@............@@@@.....@@@@@..@@@@@@..@@@@@...@@@@.....@@@@.......@@@@......
;; .@@@@@@@@@@@@@....@@@@@@@..@@@@@@@@@@@@@@@@@@@@...@@@@.....@@@@@@...@@@@@.......
;; ..@@@@@@@@@@@@@...@@@@@@@@..@@@@@@@@@@@@@@@@@@....@@@@......@@@@@@@@@@@@@.......
;; ...@@@@@@@@@@@@....@@@@@@@...@@@@@@@..@@@@@@@.....@@@@.......@@@@@@@@@@@........
;; .....@@@@@@@@@......@@@@@.....@@@@@....@@@@@.......@@..........@@@@@@@..........
;;

T1660:	; górna połowa loga
    DB 0x03,0x1f,0x3f,0x7f,0x7c,0xf8,0xff
    DB 0xfc,0xfe,0xfe,0xfc,0x00,0x00,0xfe
    DB 0x18,0x3c,0x3c,0x3c,0x3c,0x3c,0x3c
    DB 0x0c,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e
    DB 0x0c,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e
    DB 0x0c,0x1e,0x1e,0x1e,0x1e,0x1e,0x1e
    DB 0x0f,0x1f,0x3f,0x3f,0x3c,0x3c,0x3c
    DB 0x81,0xc3,0xcf,0x9f,0x1e,0x3c,0x3c
    DB 0xfc,0xff,0xff,0x8f,0x03,0x01,0x01
    DB 0x00,0x00,0x80,0xc0,0xc0,0xe0,0xe0
T16A6:  ; dolna połowa loga
    DB 0xff,0xf8,0x7c,0x7f,0x3f,0x1f,0x07
    DB 0xfe,0x00,0x00,0xfc,0xfe,0xfe,0xfc
    DB 0x3c,0x3c,0x3c,0x3f,0x3f,0x1f,0x0f
    DB 0x1e,0x1e,0x1f,0x9f,0xcf,0xc7,0x83
    DB 0x1e,0x1e,0x3f,0xff,0xff,0xf3,0xe1
    DB 0x1e,0x1e,0x3e,0xfe,0xfc,0xf8,0xf0
    DB 0x3c,0x3c,0x3c,0x3c,0x3c,0x3c,0x18
    DB 0x3c,0x3c,0x1e,0x1f,0x0f,0x07,0x01
    DB 0x01,0x01,0x03,0x8f,0xff,0xff,0xfc
    DB 0xe0,0xe0,0xc0,0xc0,0x80,0x00,0x00

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;  .@@.....@..@....@@@...@@@...@@..@...@...
;;  @......@@..@....@..@..@..@.@..@.@@.@@...
;;  @.....@.@..@..@.@@@...@..@.@..@.@.@.@...
;;  @@@..@..@..@.@..@..@..@@@..@@@@.@...@...
;;  @..@.@@@@..@@...@..@..@.@..@..@.@...@...
;;  @..@....@..@.@..@..@..@..@.@..@.@...@...
;;  .@@....@@@.@..@.@@@...@..@.@..@.@...@...
;;
T16EC:
    DB 0x60,0x81,0x82,0xe4,0x97,0x90,0x61
    DB 0x90,0x90,0x92,0x94,0x98,0x94,0xd2
    DB 0xe3,0x92,0xe2,0x93,0x92,0x92,0xe2
    DB 0x8c,0x52,0x52,0x9e,0x92,0x52,0x52
    DB 0x88,0xd8,0xa8,0x88,0x88,0x88,0x88
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;;  .@@.....@..@....@@@...@@@...@@..@...@...
;;  @..@...@@..@....@..@..@..@.@..@.@@.@@...
;;  ...@..@.@..@..@.@@@...@..@.@..@.@.@.@...
;;  ..@..@..@..@.@..@..@..@@@..@..@.@...@...
;;  .@...@@@@..@@...@..@..@.@..@..@.@...@...
;;  @.......@..@.@..@..@..@..@.@..@.@...@...
;;  @@@@...@@@.@..@.@@@...@..@..@@..@...@...
;;
T1709:
    DB 0x60,0x91,0x12,0x24,0x47,0x80,0xf1
    DB 0x90,0x90,0x92,0x94,0x98,0x94,0xd2
    DB 0xe3,0x92,0xe2,0x93,0x92,0x92,0xe2
    DB 0x8c,0x52,0x52,0x92,0x92,0x52,0x4c
    DB 0x88,0xd8,0xa8,0x88,0x88,0x88,0x88
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; ..................@........@............
;; @...@..@@...@..@@...@@@....@.....@@..@@@
;; @...@.@..@.@..@...@....@...@@@..@..@...@
;; @...@.@@@@.@...@..@..@@@...@..@.@@@@..@.
;; @.@.@.@....@....@.@.@..@...@..@.@....@..
;; .@.@...@@..@..@@..@.@@@@...@@@...@@.@@@@
;; .................@......................
;;
L1732:
    DB 0x00,0x89,0x8a,0x8b,0xaa,0x51,0x00 
    DB 0x00,0x89,0x52,0xd1,0x10,0x93,0x00 
    DB 0x20,0x8e,0x21,0x27,0xA9,0x2F,0x40
    DB 0x10,0x10,0x1C,0x12,0x12,0x1C,0x00
    DB 0x00,0x67,0x91,0xf2,0x84,0x67,0x00

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; .....................@...... 
;; @...@..@@...@@..@@.....@@@.. 
;; @...@.@..@.@...@.....@....@. 
;; @...@.@@@@.@....@@...@..@@@. 
;; @.@.@.@....@......@..@.@..@. 
;; .@.@...@@..@...@@@...@.@@@@. 
;; ...................@@....... 
;;
L1755:
    DB 0x00,0x89,0x8a,0x8b,0xaa,0x51,0x00 
    DB 0x00,0x8c,0x51,0xd0,0x10,0x91,0x00 
    DB 0x04,0xc1,0x04,0xc4,0x25,0xc5,0x18
    DB 0x00,0xc0,0x20,0xe0,0x20,0xe0,0x00

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; ....@............@......................
;; ....@..@..@..@@..@..@..@@..@...@.@@@....
;; ..@@@..@..@.@....@.@..@..@.@...@....@...
;; .@..@..@..@..@@..@@...@..@.@...@..@@@...
;; .@..@...@@@....@.@.@..@..@.@.@.@.@..@...
;; ..@@.@....@.@@@..@..@..@@...@.@..@@@@...
;; ........@@..............................
;;
 
L1771:
    DB 0x08,0x09,0x39,0x49,0x48,0x34,0x00
    DB 0x00,0x26,0x28,0x26,0xe1,0x2e,0xc0
    DB 0x40,0x49,0x52,0x62,0x52,0x49,0x00
    DB 0x00,0x91,0x51,0x51,0x55,0x8a,0x00
    DB 0x00,0x70,0x08,0x38,0x48,0x78,0x00


1786 NOP                   
1787 SUB       A,C         
1788 LD        D,C         
1789 LD        D,C         
178a LD        D,L         
178b ADC       A,D         
178c NOP                   
178d NOP                   
178e LD        (HL  ),B 
178f EX        AF,AF
1790 JR        C,0x17da       
1792 LD        A,B         
1793 NOP                   

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Dalszy ciąg procedury RST #00
;;

L1794:
; ponieważ banki ROM podczas obliczania sumy kontrolnej ulegają przełączeniu, musi nastąpić 
; relokacja procedury testu pamięci ROM, do RAM
     LD        DE,0x5b00        ; adres w RAM
     LD        HL,T17E7         ; adres kodu relokowanego
     LD        BC,0x002d        ; rozmiar procedury
     LDIR                  	; relokacja
; logo oraz test pamięci
     LD        HL,0x5800        ; wiersz 0
     CALL      0x1829           ; ustawia atrybut dla 11 znaków (logo ELWRO - pap:biały, ink: cyan)
     LD        L,0x20           ; wiersz 1
     CALL      0x1829           ; ustawia atrybut dla 11 znaków (logo ELWRO - atrybuty jw )
     LD        BC,0x0639        ; 6 znaków o atrybucie: pap: biały, ink:niebieski
     CALL      0x182c           ; atrybut dla "64kB RAM"
     LD        BC,0x053b        ; 5 znaków o atrybucie: pap: biały, ink: fioletowy
     CALL      0x182c        	; atrybut dla "24kB ROM"
     LD        BC,0x0a3c        ; 10 znaków atrybut o atrybucie: pap:biały, ink:zielony
     CALL      0x182c        	; atrybut dla "wersja dyskowa" lub "wersja bezdyskowa"
     LD        HL,T1660     	; tablica z bitmapą logo "ELWRO"
     LD        DE,0x4001        ; współrzędne ekranu (8,0) pixela
     CALL      0x00b8           ; wyświetlenie górnej połowy loga 
     LD        DE,0x4021        ; współrzędne ekranu (8,8) pixela
     CALL      0x00b8           ; wyświetlenie dolnej połowy loga 
     CALL      0x1813           ; test pamięci RAM od adresu 0x2000...0xffff
     LD        DE,0x402c        ; współrzędne ekranu (8,96) pixela
     CALL      0x00a7           ; wyświetla komunikat: "64kB RAM"
     CALL      0x5b00           ; test pamięci ROM 24kB
     CALL      0x00a7           ; wyświetla komunikat: "24kB ROM"
; wyzerowanie obszaru RAM po procedurze testu ROM
     LD        HL,0x5b00        
     LD        B,0x7f       
L17DF:
     LD        (HL   ),0x00  
     INC       HL        
     DJNZ      L17DF         
     JP        0x0040           ; dalsza część procedury pod adresem #40


;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; 
;; Funkcja:
;;  Test pamięci ROM. Funkcja oblicza sumę modulo 256 z całego obszaru 24kB ROM, a następnie porównuje
;;  wynik z bajtem pamięci # 1fff ROM BOOT, gdzie jest zapisana suma kontrolna.
;;  Jeśli bajt sumy kontrolnej jest 0xff, to wynik testu jest ignorowany.
;;
;; Jeśli wynik testu jest pozytywny, funkcja wykonuje ret i wraca do kodu z którego została wywołana.
;; Jeśli wynik testu jest negatywny to wykonywana jest instrukcja HALT i następuje zawieszenie CPU
;;
;; Wejście: brak
;; Wyjście: brak
;; Modyfikuje: AF, AF', HL'
;;

T17E7:
L5B00:
     EXX                    ; zachowaj rejestry bieżące
     XOR       A            ; suma = 0
     LD        HL,0x1ffe    ; HL= 1ffe - koniec ROM-1 (ostatni bajt to suma kontrolna)
     EX        AF,AF        ; zachowanie rejestru AF
; oblicz sumę kontrolną w BOOT ROM
     CALL      L5b23        ; testuj ROM
     LD        A,0x08       ; 
     OUT       0xf7,A       ; przełączenie na ROM ZX BAS 16k
; dodaj do niej sumę kontrolną w BAS ROM
     LD        HL,0x3fff    ; koniec pamięci ROM ZX BAS 16k
     CALL      L5b23        ; testuj ROM
     LD        A,0x48      
     OUT       0xf7,A       ; przełączenie się z powrotem na BOOT ROM
     LD        HL,T1fff     ; adres ostatniej komórki BOOT ROM (suma kontrolna modulo 256 )
     EX        AF,AF        ; przywrócenie obliczonej sumy kontrolnej w A
     CP        (HL  )       ; porównaj z bajtem sumy kontrolnej
     LD        A,(HL  )     ; pobierz bajt sumy kontrolnej
     EXX                    ; odtwórz rejestry
     RET       Z            ; jeśli suma obliczona zgadza się z sumą zapisaną w ROM - ROM OK, wyjdź
     CP        0xff         ; jeśli suma kontrolna jest równa 0xFF 
     RET       Z            ; (brak sumy kontrolnej w ROM) - OK, wyjdź 
     HALT                   ; wprowadź CPU w stan HALT - błąd w pamięci ROM

; liczenie sumy kontrolnej modulo 256 
L5B23:
     EX        AF,AF      ; zachowaj znaczniki i rejestr A
     ADD       A,(HL  )   ; suma += *HL
     DEC       HL         ; HL--
     EX        AF,AF      ; odtwórz znaczniki
     LD        A,H        ;  
     INC       A          ; 
     JR        NZ,L5B23   ; if H != 0xff goto l5b23
     RET                   
     
     
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Funkcja:
;;  Test pamięci RAM, w przypadku błędu w RAM CPU wprowadzany jest w stan HALT,
;;  Jeśli test przejdzie pomyślnie następuje powrót z funkcji
;;  Testowana jest pamięć o adresie 0x2000...0xffff
;;
;; Wejście: brak
;; Wyjście: brak
;;

L1813:
     EXX                   ; zachowuje bieżące rejestry
     LD        HL,0x2000   ; HL=0x2000 - pierwszy adres RAM
     LD        C,L         ; C=0;
L1818:
     LD        B,(HL  )    ; B = (HL)	zapamiętaj wartość komórki
     LD        (HL  ),C    ; (HL) = C   wyzeruj komórkę pamięci
     LD        A,(HL  )    ; A = (HL)   pobierz jej wartość
     CPL                   ; A = ~A    
     LD        (HL  ),A    ; (HL) = A   zaneguj i zapisz z powrotem
     LD        C,(HL  )    ; C = (HL)   pobierz wartość komórki
     LD        (HL  ),B    ; (HL) = B   odtwórz wartość komórki
     INC       HL          ; HL++
     LD        A,H         ; A=H
     OR        L           ; A |=L
     JR        NZ,0x1818   ; if (HL != 0) goto L1818 - idż aż do końca ram
     LD        A,C         ; przy sprawnej pamięci i parzystej ilości komórek, będzie wartość 0
     AND       A           ; w przypadku uszkodzenia którejkolwiek komórki wartość A != 0
     EXX                   ; odtwórz bieżące rejestry
     RET       Z           ; wyjdź jeśli A==0  - RAM OK
     HALT                  ; wejście CPU w stan HALT

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; Funkcja:
;;    Wypełnia obszar 11 bajtowy <HL+0, HL+10> wartością #3E
;;    służy do  wypełniania pola atrybutów ekranu
;;    #3E - paper 7 (White), INK 6 (Cyan)
;; Parametry wejściowe
;;  HL - adres obszaru atrybutów
;; Wyjście: brak
;;
L1829:
     LD        BC,0x0b3e     

L182C:
     LD        (HL  ),C 
     INC       HL        
     DJNZ      L182C         
     RET                   


1831 RST       56         
1832 RST       56         
1832 LD        IX,0xfe00     
1836 CALL      0x1e14        
1839 JP        C,0x1d45      
183c LD        C,0x00       
183e CALL      0xce0c        


1842 LD        DE,0x2020     
1845 ADD       HL,DE     
1846 PUSH      HL        
1847 LD        HL,0x2021     
184a CALL      0x1fe5        
184d LD        DE,(0x3fc1)   
1851 LD        HL,0xfe00     
1854 LD        A,E         
1855 OR        D           
1856 JP        Z,0x1fde      
1859 LD        C,(HL  ) 
185a CALL      0x1ff4        
185d DEC       DE          
185e INC       HL        
185f JR        0x1854         


1861 LD        A,0x48       
1863 OUT       0xf7,A       
1865 LD       (0x3ffe),SP    
1869 LD        SP,0x3ffe     
186c LD        A,0x08       
186e LD        (0x3fcf),A    
1871 CALL      0x1d25        
1874 LD        SP,(0x3ffe)   
1878 LD        A,0xc8       
187a OUT       0xf7,A       
187c JP        0x398c        
187f LD        A,0x48       
1881 OUT       0xf7,A       
1883 LD        (0x3fca),HL   
1886 LD        HL,0x3eb0     


1889 LD        A,(0x3fcc)    
188c INC       A           
188d LD        (0x3fcc),A    
1890 DEC       A           
1891 ADD       A,L         
1892 LD        L,A         
1893 LD        A,H         
1894 ADC       A,0x00       
1896 LD        H,A         
1897 POP       AF          
1898 PUSH      AF          
1899 LD        (HL  ),A 
189a LD        HL,(0x3fca)   
189d LD        A,0xc8       
189f OUT       0xf7,A       


18a1 LD        A,0x08       
18a3 JP        0x3b6b        
18a6 LD        A,B         
18a7 LD        (0x5c81),A    
18aa LD        A,0x08       
18ac LD        (0x3fcf),A    
18ae PUSH      IX        
18b0 
18b1 LD        IX,0x3fb6     
18b4 
18b5 LD        (U),0x04  
18b8 LD        IX,0x3fcc     
18bb LD        A,(IX+36) 
18bd NOP                   
18be 


18c0 LD        (HL  ),A 
18c1 DEC       BC          
18c1 LD        (U),0x00  
18c5 LD        IX,0x3eb0     
18c8 CALL      0x1b89        
18cb 
18cc POP       IX        
18cd LD        A,0xc8       
18cf OUT       0xf7,A       
18d1 CALL      0x39cc        
18d4 LD        A,0x08       
18d6 JP        0x0070        
18d9 PUSH      AF          
18da LD        A,(0x3fcf)    
18dd CP        0x08         


18e0 JR        NZ,0x1916      
18e2 LD        A,0x5c       
18e4 OUT       0xf7,A       
18e6 LD        A,0x48       
18e8 PUSH      AF          
18e9 LD        A,0xff       
18eb IN        A,0xfe       
18ed CPL                   
18ee AND       0x3f         
18f0 LD        C,A         
18f1 POP       AF          
18f2 OUT       0xf7,A       
18f4 POP       AF          
18f5 RET                   
18f6 SCF                   


18f7 JP        0x1915        
18fa JP        0x1d25        
18fd JP        0x1cb6        
1900 JP        0x1861        
1903 JP        0x1c3f        
1906 JP        0x1c5a        
1909 JP        0x1f59        
190c JP        0x18a6        
190f JP        0x187f        
1912 JP        0x1bbd        
1915 PUSH      AF          
1916 LD        A,0x7c       
1918 OUT       0xf7,A       
191a LD        A,0x68       
191c JR        0x18e8         


191e DI                    
191f PUSH      BC          
1920 EXX                   
1921 POP       BC          
1922 DEC       BC          
1923 EXX                   
1924 CALL      0x1a82        
1927 LD        B,0x00       
1929 CALL      0x1cfc        
192c XOR       A           
192d CALL      0x19e7        
1930 LD        A,(HL  ) 
1931 INC       HL        
1932 CALL      0x19e7        
1935 LD        A,(HL  ) 


1936 INC       HL        
1937 CALL      0x19e7        
193a CALL      0x19f2        
193d EXX                   
193e DEC       BC          
193f LD        A,B         
1940 OR        C           
1941 EXX                   
1942 JR        NZ,0x1935      
1944 LD        A,B         
1945 NEG                   
1947 CALL      0x19e7        
194a CALL      0x19f2        
194d LD        E,D         
194e CALL      0x1992        


1951 CALL      0x19f2        
1954 LD        E,C         
1955 CALL      0x1992        
1958 CALL      0x19f2        
195b CALL      0x1aa8        
195e DEC       A           
195f JR        NZ,0x195e      
1961 AND       A           
1962 JR        0x19ad         
1964 DI                    
1965 PUSH      BC          
1966 EXX                   
1967 POP       BC          
1968 EXX                   
1969 CALL      0x1a82        


196c LD        B,0x00       
196e CALL      0x1cfc        
1971 XOR       A           
1972 CALL      0x19e7        
1975 LD        A,0x0f       
1977 CALL      0x19e7        
197a EXX                   
197b LD        A,C         
197c EXX                   
197d CALL      0x19e7        
1980 CALL      0x19f2        
1983 EXX                   
1984 LD        A,B         
1985 INC       BC          
1986 EXX                   


1987 JR        0x1937         
1989 PUSH      AF          
198a IN        A,0xbf       
198c AND       0x05         
198e JR        Z,0x198a       
1990 POP       AF          
1991 RET                   
1992 PUSH      AF          
1993 IN        A,0xbf       
1995 BIT       7,A         
1997 JR        NZ,0x19dd      
1999 AND       0x02         
199b JR        Z,0x1993       
199d POP       AF          
199e RET                   


199f LD        A,0x94       
19a1 OUT       0xbf,A       
19a3 IN        A,0xbf       
19a5 BIT       7,A         
19a7 JR        NZ,0x19de      
19a9 BIT       6,A         
19ab JR        Z,0x19a3       
19ad PUSH      AF          
19ae LD        A,(0x3fce)    
19b1 OR        0x14         
19b3 OUT       0xbf,A       
19b5 POP       AF          
19b6 RET                   
19b7 IN        A,0xbf       
19b9 BIT       7,A         


19bb JR        NZ,0x19de      
19bd AND       0x02         
19bf JR        Z,0x19b7       
19c1 IN        A,0xbe       
19c3 AND       A           
19c4 JR        NZ,0x19b7      
19c6 IN        A,0xbf       
19c8 BIT       7,A         
19ca JR        NZ,0x19de      
19cc AND       0x02         
19ce JR        Z,0x19c6       
19d0 IN        A,0xbe       
19d2 LD        (HL  ),A 
19d3 ADD       A,B         
19d4 LD        B,A         


19d5 INC       HL        
19d6 IN        A,0xbf       
19d8 AND       0x08         
19da JR        NZ,0x19de      
19dc RET                   
19dd POP       AF          
19de POP       AF          
19df SCF                   
19e0 LD        A,0xff       
19e2 LD        (0x3fd0),A    
19e5 JR        0x19ad         
19e7 CALL      0x1989        
19ea OUT       0xbe,A       
19ec LD        E,D         
19ed LD        D,C         


19ee LD        C,A         
19ef ADD       A,B         
19f0 LD        B,A         
19f1 RET                   
19f2 IN        A,0xbe       
19f4 CP        E           
19f5 JR        NZ,0x19de      
19f7 RET                   
19f8 DI                    
19f9 LD        HL,0x3fb0     
19fc LD        E,0x00       
19fe LD        B,0x10       
1a00 DJNZ      0x1a00         
1a02 LD        B,0x00       
1a04 CALL      0x199f        


1a07 CALL      0x19b7        
1a0a DEC       HL        
1a0b LD        A,(HL  ) 
1a0c INC       HL        
1a0d CP        0x0f         
1a0f JR        Z,0x1a2d       
1a11 CALL      0x19c6        
1a14 CALL      0x19c6        
1a17 CALL      0x1992        
1a1a IN        A,0xbe       
1a1c ADD       A,B         
1a1d PUSH      AF          
1a1e IN        A,0xbf       
1a20 AND       0x08         
1a22 JR        NZ,0x19de      


1a24 CALL      0x1aa8        
1a27 POP       AF          
1a28 JP        NZ,0x19df     
1a2b AND       A           
1a2c RET                   
1a2d CALL      0x19c6        
1a30 DEC       HL        
1a31 LD        E,(HL  ) 
1a32 INC       HL        
1a33 CALL      0x19c6        
1a36 DEC       HL        
1a37 LD        D,(HL  ) 
1a38 INC       HL        
1a39 PUSH      DE          
1a3a EXX                   


1a3b POP       BC          
1a3c EXX                   
1a3d LD        E,0xff       
1a3e PUSH      IX        
1a40 POP       IX        
1a41 CALL      0x19c6        
1a44 EXX                   
1a45 DEC       DE          
1a46 DEC       BC          
1a47 LD        A,B         
1a48 OR        C           
1a49 EXX                   
1a4a JR        NZ,0x1a42      
1a4c JR        0x1a17         
1a4e DI                    


1a50 LD        B,0x00       
1a52 LD        E,0x00       
1a54 LD        A,0xff       
1a56 LD        (0x3fd0),A    
1a59 CALL      0x18da        
1a5c CALL      0x199f        
1a5f CALL      0x19b7        
1a62 DEC       HL        
1a63 LD        A,(HL  ) 
1a64 LD        (0x3fd1),A    
1a67 INC       HL        
1a68 CP        0xff         
1a6a JR        Z,0x1a7b       
1a6c CP        C           
1a6d JR        Z,0x1a7b       


1a6f CP        0x40         
1a71 JP        C,0x19df      
1a74 AND       0x3f         
1a76 OR        C           
1a77 CP        C           
1a78 JP        NZ,0x19df     
1a7b EXX                   
1a7c LD        BC,0x0016     
1a7f EXX                   
1a80 JR        0x1a42         
1a82 PUSH      AF          
1a83 LD        A,0xb5       
1a85 OUT       0xbf,A       
1a87 EX (SP),HL            
1a88 OUT       0xbe,A       


1a8a EX (SP),HL            
1a8b LD        BC,0xffff     
1a8e IN        A,0xbf       
1a90 AND       0x40         
1a92 JR        NZ,0x1a9c      
1a94 DEC       BC          
1a95 LD        A,B         
1a96 OR        C           
1a97 JP        Z,0x19de      
1a9a JR        0x1a8e         
1a9c LD        A,0x35       
1a9e OUT       0xbf,A       
1aa0 PUSH      AF          
1aa1 POP       AF          
1aa2 IN        A,0xbe       


1aa4 IN        A,0xbe       
1aa6 POP       AF          
1aa7 RET                   
1aa8 LD        A,(0x3fce)    
1aab CPL                   
1aac AND       0x34         
1aae OUT       0xbf,A       
1ab0 RET                   
1ab1 IN        A,0xbf       
1ab3 AND       0x80         
1ab5 RET       NZ          
1ab6 LD        A,R         
1ab8 CALL      0x1cfc        
1abb DEC       A           
1abc JR        NZ,0x1ab8      


1abe JR        0x1ab1         
1ac0 XOR       A           
1ac1 LD        (0x3fce),A    
1ac4 LD        HL,0x3fb0     
1ac7 LD        (HL   ),0x06  
1ac9 LD        BC,0x0003     
1acc CALL      0x191e        
1acf JR        C,0x1b02       
1ad1 CALL      0x19f8        
1ad4 JR        C,0x1aeb       
1ad6 LD        A,E         
1ad7 INC       A           
1ad8 JR        Z,0x1af9       
1ada LD        HL,0x3fb0     
1add LD        A,(HL  ) 


1ade CP        0x08         
1ae0 JR        Z,0x1afe       
1ae2 CALL      0x1b05        
1ae5 CP        0x05         
1ae7 JR        Z,0x1ac0       
1ae9 JR        0x1aee         
1aeb CALL      0x1b05        
1aee LD        (HL   ),0x05  
1af0 INC       HL        
1af1 INC       HL        
1af2 LD        (HL   ),0xff  
1af4 LD        HL,0x3fb0     
1af7 JR        0x1ac9         
1af9 PUSH      HL        
1af9 POP       IX        


1afc JR        0x1ac0         
1afe AND       A           
1aff JP        0x19ad        
1b02 SCF                   
1b03 JR        0x1aff         
1b05 PUSH      AF          
1b06 PUSH      HL        
1b06 DEC       IY        
1b08 ILLEGAL              
1b09 PUSH      IY        
1b0a POP       IY        
1b0b LD        A,H         
1b0c OR        L           
1b0d POP       IY        
1b0e JR        Z,0x1b13       


1b11 POP       AF          
1b12 RET                   
1b13 POP       AF          
1b14 POP       AF          
1b15 JR        0x1b02         
1b17 CALL      0x19f8        
1b1a JR        C,0x1b02       
1b1c LD        HL,0x3fb0     
1b1f LD        A,(HL  ) 
1b20 CP        0x06         
1b22 JR        Z,0x1b3d       
1b24 CALL      0x1b05        
1b27 LD        HL,0x3fb0     
1b2a LD        (HL   ),0x05  
1b2c INC       HL        


1b2d INC       HL        
1b2e LD        (HL   ),0xff  
1b30 LD        HL,0x3fb0     
1b33 LD        BC,0x0003     
1b36 CALL      0x191e        
1b39 JR        C,0x1b02       
1b3b JR        0x1b17         
1b3d XOR       A           
1b3e LD        (0x3fce),A    
1b40 PUSH      IX        
1b42 POP       IX        
1b43 EXX                   
1b44 PUSH      DE          
1b45 EXX                   
1b46 POP       BC          


1b48 CALL      0x1964        
1b4b JR        C,0x1b02       
1b4d CALL      0x19f8        
1b50 JR        C,0x1b63       
1b52 LD        HL,0x3fb0     
1b55 LD        A,(HL  ) 
1b56 CP        0x06         
1b58 JR        Z,0x1b70       
1b5a CALL      0x1b05        
1b5d CP        0x05         
1b5f JR        Z,0x1b3d       
1b61 JR        0x1b66         
1b63 CALL      0x1b05        
1b66 LD        (HL   ),0x05  
1b68 LD        BC,0x0003     


1b6b CALL      0x191e        
1b6e JR        0x1b4b         
1b70 CALL      0x1b79        
1b73 CALL      0x191e        
1b76 JP        0x19ad        
1b79 LD        HL,0x3fb0     
1b7c LD        (HL   ),0x08  
1b7e INC       HL        
1b7f INC       HL        
1b80 LD        (HL   ),0x00  
1b82 LD        HL,0x3fb0     
1b85 LD        BC,0x0003     
1b88 RET                   
1b89 DI                    
1b8a EXX                   


1b8b PUSH      BC          
1b8c PUSH      DE          
1b8d PUSH      HL        
1b8e EXX                   
1b8e PUSH      IY        
1b90 PUSH      IY        
1b91 CALL      0x18da        
1b94 LD        A,(0x5c81)    
1b97 LD        B,A         
1b98 CP        0x40         
1b9a JR        C,0x1ba3       
1b9c LD        A,C         
1b9d CP        0x10         
1b9f JP        NC,0x1c1d     
1ba2 LD        IY,0x3fb0     


1ba6 LD        (HL  ),B 
1ba7 INC       HL        
1ba8 LD        (HL  ),C 
1ba9 INC       HL        
1baa LD        (HL   ),0x0e  
1bac INC       HL        
1bad INC       HL        
1bae INC       HL        
1baf LD        A,(0x3fc9)    
1bb2 LD        (HL  ),A 
1bb3 INC       HL        
1bb3 PUSH      IX        
1bb5 POP       DE          
1bb7 EX        DE,HL       
1bb8 LD        BC,0x0011     


1bbb LDIR                  
1bbc LD        IY,0x0002     
1bc0 XOR       A           
1bc1 LD        (0x3fce),A    
1bc4 CALL      0x19ad        
1bc7 CALL      0x1ab1        
1bca CALL      0x1aa8        
1bcd LD        A,(0x5c81)    
1bd0 CP        0x40         
1bd2 JR        C,0x1bd9       
1bd4 LD        A,0x20       
1bd6 JR        0x1bda         
1bd8 XOR       A           
1bd9 LD        (0x3fce),A    
1bdc LD        IY,0x3fb0     


1be0 LD        BC,0x0017     
1be3 CALL      0x191e        
1be6 JR        NC,0x1bed      
1be8 CALL      0x1d1d        
1beb JR        0x1bc1         
1bed LD        A,(0x3fb0)    
1bf0 CP        0x40         
1bf2 JR        C,0x1c10       
1bf4 POP       HL        
1bf4 LD        C,(IX+0b) 
1bf7 
1bf8 LD        B,(IX+0c) 
1bfa PUSH      IX        
1bfb CALL      0x1cfc        
1bfe CALL      0x1964        


1c02 JR        C,0x1bc1       
1c04 POP       HL        
1c05 CALL      0x1b79        
1c08 CALL      0x1cfc        
1c0b CALL      0x191e        
1c0e JR        0x1c2e         
1c10 CALL      0x19f8        
1c13 JR        C,0x1c1d       
1c15 LD        HL,0x3fb0     
1c18 LD        A,(HL  ) 
1c19 CP        0x0d         
1c1b JR        Z,0x1c21       
1c1d POP       HL        
1c1e SCF                   
1c1f JR        0x1c2e         


1c21 EXX                   
1c21 LD        E,(IX+0b) 
1c24 
1c25 LD        D,(IX+0c) 
1c27 EXX                   
1c28 
1c29 POP       IX        
1c2a CALL      0x1b3d        
1c2d PUSH      AF          
1c2e XOR       A           
1c2f LD        (0x3fce),A    
1c32 CALL      0x19ad        
1c35 POP       AF          
1c36 ILLEGAL              
1c37 POP       IX        


1c39 EXX                   
1c3a POP       HL        
1c3b POP       DE          
1c3c POP       BC          
1c3d EXX                   
1c3e RET                   
1c3f LD        A,0x48       
1c41 OUT       0xf7,A       
1c43 LD        A,0x02       
1c45 LD        (0x3fc9),A    
1c48 LD        A,0x08       
1c4a LD        (0x3fcf),A    
1c4d CALL      0x1b89        
1c50 LD        A,0xc8       
1c52 OUT       0xf7,A       


1c54 CALL      0x39cc        
1c57 JP        0x38fa        
1c5a DI                    
1c5b EXX                   
1c5c PUSH      BC          
1c5d PUSH      DE          
1c5e PUSH      HL        
1c5f EXX                   
1c5f PUSH      IY        
1c61 PUSH      DE          
1c62 EXX                   
1c63 POP       DE          
1c64 EXX                   
1c65 LD        D,C         
1c66 LD        A,0x48       


1c69 OUT       0xf7,A       
1c6b LD        A,0x08       
1c6d LD        (0x3fcf),A    
1c70 CALL      0x18da        
1c73 LD        A,(0x5c81)    
1c76 LD        B,A         
1c77 CP        0x40         
1c79 JP        NC,0x1d09     
1c7c LD        HL,0x3fb0     
1c7f LD        (HL  ),B 
1c80 INC       HL        
1c81 LD        (HL  ),C 
1c82 INC       HL        
1c83 LD        (HL   ),0x0e  
1c85 INC       HL        


1c86 INC       HL        
1c87 INC       HL        
1c88 LD        A,D         
1c89 AND       0x80         
1c8b OR        0x01         
1c8d LD        (HL  ),A 
1c8e INC       HL        
1c8f LD        A,D         
1c90 AND       A           
1c91 JR        NZ,0x1c9e      
1c92 PUSH      IX        
1c94 POP       DE          
1c95 LD        BC,0x0011     
1c99 EX        DE,HL       
1c9a SBC       HL,BC     


1c9c LDIR                  
1c9e CALL      0x1cb6        
1ca0 POP       IY        
1ca2 EXX                   
1ca3 POP       IY        
1ca4 POP       DE          
1ca5 POP       BC          
1ca6 EXX                   
1ca7 LD        A,0xc8       
1ca9 OUT       0xf7,A       
1cab EI                    
1cac HALT                  
1cad EI                    
1cae HALT                  
1caf CALL      0x39cc        


1cb3 JP        0x3938        
1cb5 LD        IY,0x0002     
1cb9 XOR       A           
1cba LD        (0x3fce),A    
1cbd CALL      0x19ad        
1cc0 LD        IY,0x3fb0     
1cc3 LD        BC,0x0017     
1cc6 CALL      0x1ab1        
1cc9 CALL      0x1aa8        
1ccc CALL      0x191e        
1ccf JR        NC,0x1cd7      
1cd1 CALL      0x1d04        
1cd4 JR        0x1cba         
1cd6 CALL      0x19f8        
1cd9 JR        C,0x1d09       


1cdc LD        HL,0x3fb0     
1cdf LD        A,(HL  ) 
1ce0 CP        0x0d         
1ce2 JR        NZ,0x1d09      
1ce4 CALL      0x1ac0        
1ce7 JR        C,0x1d09       
1ce9 EXX                   
1cea LD        A,D         
1ceb OR        E           
1cec EXX                   
1ced JR        NZ,0x1d09      
1cef XOR       A           
1cf0 AND       A           
1cf1 LD        A,0x00       
1cf3 LD        (0x3fce),A    


1cf6 CALL      0x19ad        
1cf9 CALL      0x1cfc        
1cfc PUSH      AF          
1cfd LD        A,0xff       
1cff DEC       A           
1d00 JR        NZ,0x1cff      
1d02 POP       AF          
1d03 RET                   
1d04 CALL      0x1d0d        
1d07 RET       NZ          
1d08 POP       AF          
1d09 XOR       A           
1d0a SCF                   
1d0b JR        0x1cf1         
1d0d LD        A,R         


1d0f CALL      0x1cfc        
1d12 DEC       A           
1d13 JR        NZ,0x1d0f      
1d14 DEC       IY        
1d16 ILLEGAL              
1d17 PUSH      IY        
1d18 POP       IY        
1d19 LD        A,H         
1d1a OR        L           
1d1b RET                   
1d1c CALL      0x1d0d        
1d1f RET       NZ          
1d20 POP       AF          
1d21 JP        0x1c1d        
1d24 XOR       A           


1d26 LD        (0x3fce),A    
1d29 PUSH      HL        
1d2a PUSH      BC          
1d2b PUSH      DE          
1d2b PUSH      IX        
1d2d ILLEGAL              
1d2e PUSH      IX        
1d2f EXX                   
1d30 PUSH      BC          
1d31 PUSH      DE          
1d32 PUSH      IX        
1d33 EXX                   
1d34 LD        B,0x10       
1d36 DJNZ      0x1d37         
1d38 ILLEGAL              


1d3a LD        HL,0x0002     
1d3d LD        HL,0x3fb0     
1d40 CALL      0x1a4f        
1d43 JR        NC,0x1d5e      
1d45 XOR       A           
1d46 LD        (0x3fce),A    
1d49 CALL      0x19ad        
1d4c LD        A,(0x3fd0)    
1d4f CP        0x01         
1d51 EXX                   
1d52 POP       HL        
1d53 POP       DE          
1d54 POP       BC          
1d55 EXX                   
1d55 POP       IY        


1d57 POP       IX        
1d59 POP       DE          
1d5a POP       BC          
1d5b POP       IX        
1d5c RET                   
1d5d LD        IX,0x3fb2     
1d60 LD        A,(IX-02) 
1d62 LD        C,0x20       
1d64 RST       24          
1d65 INC       IX        
1d66 INC       IX        
1d67 INC       IX        
1d68 LD        A,(0x3fcf)    
1d6b LD        B,A         
1d6c LD        A,(IX-02) 


1d6f LD        (BC),A      
1d70 LD        C,0x08       
1d72 JR        Z,0x1dcf       
1d74 CP        0x81         
1d76 JR        Z,0x1d89       
1d78 CP        0x01         
1d7a JR        Z,0x1da4       
1d7c CP        0x04         
1d7e JP        Z,0x1e3d      
1d81 CP        0x82         
1d83 LD        C,0x68       
1d85 JR        Z,0x1dcf       
1d87 JR        0x1d45         
1d89 CALL      0x1e89        
1d8c JR        C,0x1d45       


1d8e LD        HL,0x3fb0     
1d91 LD        (HL   ),0x0d  
1d93 LD        BC,0x0003     
1d96 CALL      0x191e        
1d99 JR        C,0x1d45       
1d9b XOR       A           
1d9c LD        (0x3fce),A    
1d9f CALL      0x1b17        
1da2 JR        0x1d45         
1da4 CALL      0x1e89        
1da6 LD        IX,0x3fb6     
1daa EXX                   
1dab 
1dac LD        (IX+0b),E 
1dae 


1db0 LD        (HL  ),D 
1db1 INC       C           
1db2 LD        A,(0x3fb6)    
1db5 AND       A           
1db6 JR        NZ,0x1dc9      
1db8 PUSH      HL        
1db9 LD        HL,(0x5c4b)   
1dbc LD        DE,(0x5c53)   
1dc0 SBC       HL,DE     
1dc1 LD        (IX+0f),L 
1dc4 
1dc5 LD        (IX+10),H 
1dc7 POP       IX        
1dc8 LD        DE,0x0011     
1dcb EXX                   


1dcd JR        0x1d8e         
1dcf LD        A,B         
1dd0 CP        C           
1dd1 JP        NZ,0x1d45     
1dd4 LD        HL,0x3fb0     
1dd7 LD        A,(HL  ) 
1dd8 CP        0x40         
1dda JP        C,0x1d45      
1ddd LD        A,0x20       
1ddf LD        (0x3fce),A    
1de2 CALL      0x1edb        
1de5 JR        C,0x1e0c       
1de7 CALL      0x19f8        
1dea JR        C,0x1e0c       
1dec LD        A,E         


1ded INC       A           
1dee JR        NZ,0x1df5      
1df0 PUSH      HL        
1df0 POP       IX        
1df2 JR        0x1de7         
1df4 LD        IX,0x3fb0     
1df7 LD        A,(IX-02) 
1df9 EX        AF,AF
1dfa JR        NZ,0x1e0c      
1dfc INC       IX        
1dfd INC       IX        
1dfe LD        A,(IX-59) 
1e00 JR        NZ,0x1e0c      
1e02 EXX                   
1e03 LD        A,D         


1e05 OR        E           
1e06 EXX                   
1e07 JR        NZ,0x1e0c      
1e09 JP        0x1d45        
1e0c LD        A,0xff       
1e0e LD        (0x3fd0),A    
1e11 JP        0x1d45        
1e14 EXX                   
1e15 LD        DE,(0x3fc1)   
1e19 EXX                   
1e1a LD        HL,0x3fb0     
1e1d LD        A,(0x3fd1)    
1e20 CP        0x40         
1e22 LD        A,0x00       
1e24 JR        C,0x1e28       


1e26 LD        A,0x20       
1e28 LD        (0x3fce),A    
1e2b AND       A           
1e2c JR        NZ,0x1e33      
1e2e LD        (HL   ),0x0d  
1e30 JP        0x1ac9        
1e33 CALL      0x19f8        
1e36 RET       C           
1e37 LD        A,E         
1e38 INC       A           
1e39 SCF                   
1e3a RET       NZ          
1e3b AND       A           
1e3c RET                   
1e3d LD        A,(0x3fcf)    


1e40 CP        0x08         
1e42 JP        NZ,0x1833     
1e44 LD        IX,0x3db0     
1e48 CALL      0x1e14        
1e4b JP        C,0x1d45      
1e4e LD        DE,(0x3fc1)   
1e52 LD        IX,0x5c3b     
1e55 LD        A,(IX-0b) 
1e57 LD        (U),0x23  
1e5a LD        A,(IX-0b) 
1e5c LD        (U),0xe5  
1e5f LD        IX,0x3db0     
1e62 LD       (0x3fca),SP    
1e66 LD        SP,(0x3ffe)   
1e6a ILLEGAL              


1e6c LD        HL,0x5c3a     
1e6f LD        A,0xc8       
1e71 OUT       0xf7,A       
1e73 LD        A,0x08       
1e75 CALL      0x3af3        
1e78 LD        A,0x48       
1e7a OUT       0xf7,A       
1e7c LD        SP,(0x3fca)   
1e80 POP       HL        
1e81 POP       AF          
1e82 LD        (HL  ),A 
1e83 DEC       HL        
1e84 POP       AF          
1e85 LD        (HL  ),A 
1e86 JP        0x1d45        


1e89 LD        HL,0x3fb6     
1e8c PUSH      HL        
1e8c POP       IX        
1e8e LD        A,(IX-59) 
1e90 JR        Z,0x1e9e       
1e92 DEC       A           
1e93 JR        Z,0x1ec4       
1e95 DEC       A           
1e96 JR        Z,0x1ed2       
1e98 DEC       A           
1e99 JR        Z,0x1eb4       
1e9b SCF                   
1e9c RET                   
1e9d LD        BC,(0x5c53)   
1ea1 LD        IX,(0x5c59)   


1ea5 DEC       HL        
1ea6 AND       A           
1ea7 SBC       HL,BC     
1ea9 PUSH      HL        
1eaa EXX                   
1eab POP       DE          
1eac EXX                   
1ead PUSH      BC          
1eae POP       HL        
1eaf PUSH      HL        
1eaf POP       IX        
1eb1 AND       A           
1eb2 RET                   
1eb3 EXX                   
1eb4 


1eb6 LD        E,(HL  ) 
1eb7 DEC       BC          
1eb7 LD        D,(IX+0c) 
1eba EXX                   
1ebb 
1ebc LD        L,(IX+0d) 
1ebe 
1ebf LD        H,(IX+0e) 
1ec1 JR        0x1eaf         
1ec3 
1ec4 LD        A,(IX+0e) 
1ec6 AND       0x1f         
1ec8 OR        0x80         
1eca LD        B,A         
1ecb CALL      0x1f9a        


1ecf RET       C           
1ed0 JR        0x1eaf         
1ed1 LD        A,(IX+0e) 
1ed4 AND       0x1f         
1ed6 OR        0xc0         
1ed8 JR        0x1ecb         
1eda LD        IX,0x3fb6     
1edd PUSH      IX        
1ede 
1edf POP       IX        
1ee0 EXX                   
1ee1 
1ee2 LD        D,(IX+0c) 
1ee4 
1ee5 LD        E,(IX+0b) 


1ee8 EXX                   
1ee9 LD        A,(HL  ) 
1eea AND       A           
1eeb JR        Z,0x1f2e       
1eed DEC       A           
1eee JR        Z,0x1f00       
1ef0 DEC       A           
1ef1 JR        Z,0x1f25       
1ef3 DEC       A           
1ef4 JR        Z,0x1ef8       
1ef6 JR        0x1e9c         
1ef7 LD        L,(IX+0d) 
1efa 
1efb LD        H,(IX+0e) 
1efd JR        0x1eaf         


1eff LD        A,(IX+0e) 
1f02 AND       0x1f         
1f04 OR        0x80         
1f06 LD        B,A         
1f07 CALL      0x1f9a        
1f0a RET       C           
1f0b PUSH      IX        
1f0c INC       IX        
1f0d LD        C,(IX+23) 
1f0f LD        B,(IX+69) 
1f11 LD        H,B         
1f12 INC       IX        
1f13 INC       IX        
1f14 INC       IX        
1f15 EXX                   


1f17 PUSH      DE          
1f18 EXX                   
1f19 POP       DE          
1f1a AND       A           
1f1b SBC       HL,DE     
1f1d LD        A,H         
1f1e OR        L           
1f1f POP       HL        
1f20 JP        NZ,0x1e9c     
1f23 JR        0x1eaf         
1f24 LD        A,(IX+0e) 
1f27 AND       0x1f         
1f29 OR        0xc0         
1f2b JR        0x1f07         
1f2d XOR       A           


1f2f LD        (0x3fd0),A    
1f32 LD       (0x3fe0),SP    
1f36 LD        SP,(0x3ffe)   
1f39 LD        C,(IX+0b) 
1f3c 
1f3d LD        B,(IX+0c) 
1f3f LD        DE,(0x5c53)   
1f43 LD        IX,(0x5c59)   
1f46 DEC       IX        
1f47 PUSH      BC          
1f48 LD        A,(0x3fcf)    
1f4b CP        0x08         
1f4d JR        NZ,0x1f92      
1f4f LD        A,0xc8       
1f51 OUT       0xf7,A       


1f54 LD        A,0x08       
1f56 JP        0x386e        
1f59 LD        A,0x48       
1f5b OUT       0xf7,A       
1f5d INC       HL        
1f5d LD        C,(IX+0f) 
1f60 
1f61 LD        B,(IX+10) 
1f63 ADD       IX,BC     
1f64 LD        (0x5c4b),IX   
1f67 
1f68 LD        H,(IX+0e) 
1f6a LD        A,H         
1f6b AND       0xc0         
1f6d JR        NZ,0x1f87      


1f6f LD        L,(IX+0d) 
1f72 LD        (0x5c42),IX   
1f75 XOR       A           
1f76 LD        (0x5c44),A    
1f79 LD        (0x5c6a),A    
1f7c LD        A,0xdd       
1f7e LD        (0x5c3b),A    
1f81 LD        A,0x01       
1f83 LD        (0x3fd0),A    
1f86 EXX                   
1f87 POP       DE          
1f88 EXX                   
1f89 POP       IX        
1f8a LD        SP,(0x3fe0)   
1f8e JP        0x1eaf        


1f92 POP       BC          
1f93 LD        HL,0x4000     
1f96 PUSH      HL        
1f97 PUSH      BC          
1f98 JR        0x1f82         
1f9a LD        HL,(0x5c4b)   
1f9d LD        A,(HL  ) 
1f9e CP        0x80         
1fa0 JR        Z,0x1fcd       
1fa2 CP        B           
1fa3 JR        Z,0x1fcf       
1fa5 AND       0xe0         
1fa7 CP        0x60         
1fa9 JR        Z,0x1fbb       
1fab CP        0xe0         


1fad JR        Z,0x1fc0       
1faf CP        0xa0         
1fb1 JR        Z,0x1fc5       
1fb3 INC       HL        
1fb4 LD        C,(HL  ) 
1fb5 INC       HL        
1fb6 LD        B,(HL  ) 
1fb7 ADD       HL,BC     
1fb8 INC       HL        
1fb9 JR        0x1f9d         
1fbb LD        BC,0x0005     
1fbe JR        0x1fb7         
1fc0 LD        BC,0x0012     
1fc3 JR        0x1fb7         
1fc5 INC       HL        


1fc6 LD        A,(HL  ) 
1fc7 AND       0x80         
1fc9 JR        Z,0x1fc5       
1fcb JR        0x1fbb         
1fcd SCF                   
1fce RET                   
1fcf PUSH      HL        
1fd0 INC       HL        
1fd1 LD        E,(HL  ) 
1fd2 INC       HL        
1fd3 LD        D,(HL  ) 
1fd4 INC       DE          
1fd5 INC       DE          
1fd6 INC       DE          
1fd7 PUSH      DE          


1fd8 EXX                   
1fd9 POP       DE          
1fda EXX                   
1fdb POP       HL        
1fdc AND       A           
1fdd RET                   
1fde POP       HL        
1fdf CALL      0x1fe5        
1fe2 JP        0x1d45        
1fe5 LD        C,0x1b       
1fe7 CALL      0x1ff4        
1fea LD        C,0x59       
1fec CALL      0x1ff4        
1fef LD        C,L         
1ff0 CALL      0x1ff4        


1ff3 LD        C,H         
1ff4 PUSH      DE          
1ff5 PUSH      HL        
1ff6 CALL      0xce0c        
1ff9 POP       HL        
1ffa POP       DE          
1ffb RET                   
1ffc RST       56         
1ffd RST       56         
1ffe RST       56         

; suma kontrolna modulo 256 z zawartości całej (24kB) pamięci ROM, jeśli 0xff tzn że nie ustawiono
T1fff:
    DB 0xff

