; Copyright INMOS Limited 1988,1990,1991
; @(#) Module: b004asm.asm, revision 1.7 of 4/5/92

                dosseg
                page    59,186
                .lall
                .model  large,c

; Define the hardware segment

HARDWARE        segment at 40h
                org     6ch

timer           dw      0

HARDWARE        ends


                .fardata

extrn           C012_idr:word
extrn           C012_odr:word
extrn           C012_isr:word
extrn           C012_osr:word

                .code

                assume  cs:@code,ds:DGROUP

timeout         dw      0
timeout_fl      db      0
last_count      dw      0

                public  B004ReadLink
B004ReadLink    proc    uses bx cx dx si di es ds,linkid:word,buffer:dword,count:word,tout:word

; Poll the CTRL-C key
		mov     ah,0bh
		int	21h

; Calculate the time at which we give up

                mov     ax,tout           ; timeout
                mov     bx,182            ; work out timeout in machine ticks
                mul     bx
                mov     bx,100
                div     bx
                mov     timeout,ax
                mov     last_count,ax

; Get parameters into registers

                mov     cx,count          ; CX = count
                les     di,buffer         ; ES:DI -> Buffer

                mov     ax,seg C012_isr
                mov     ds,ax
                
                assume  ds:nothing
                
                mov     dx,ds:C012_isr
                mov     bx,ds:C012_idr

                mov     ax,HARDWARE
                mov     ds,ax

                assume  ds:HARDWARE

                mov     ax,timer
                add     ax,timeout
                dec     ax
                mov     timeout_fl,0
                mov     si,ax

rdnext:         in      al,dx             ; read ladp status
                test    al,1              ; byte ready ?
                jz      rdnot_ready       ; no, branch
                xchg    dx,bx             ; thank you, Mr Intel !
                in      al,dx             ; read the byte...
                stosb                     ; ...and stash it
                xchg    bx,dx
                loop    rdnext            ; loop until all read
                jmp     rdok              ; all read.

; If a timeout hasn't happened get back into the polling loop

rdnot_ready:    cmp     si,timer          ; have we run out of time
                jne     rdnext

;Is the count the same as it was at the last timeout

                cmp     cx,last_count
                je      rdgiveup          ; Yes  -- exit

;Record the new count and reset the timeout

                mov     last_count,cx
                mov     ax,timer
                add     ax,timeout
                dec     ax
                mov     si,ax
                jmp     rdnext

rdgiveup:       mov     timeout_fl,2
                mov     ax,count
                sub     ax,cx
                jmp     rdterm

rdok:           mov     ax,count
rdterm:         ret

B004ReadLink    endp

                assume  ds:DGROUP
                public  B004WriteLink
B004WriteLink   proc    uses bx cx dx si di es ds,linkid:word,buffer:dword,count:word,tout:word

; Poll the CTRL-C key
		mov     ah,0bh
		int	21h

; Then do the write
                mov     ax,tout
                mov     bx,182            ; work out timeout in machine ticks
                mul     bx
                mov     bx,100
                div     bx
                mov     timeout,ax

                mov     cx,count
                lds     si,buffer

                assume  ds:nothing

                mov     last_count,cx

                mov     ax,seg C012_osr
                mov     es,ax
                mov     dx,es:C012_osr
                mov     bx,es:C012_odr

                mov     ax,HARDWARE
                mov     es,ax
                
                assume  es:HARDWARE
                
                mov     ax,timer
                add     ax,timeout
                dec     ax
                mov     timeout_fl,0
                mov     di,ax

wrnext:         in      al,dx             ; read ladp status
                test    al,1              ; ready to write ?
                jz      wrnot_ready       ; no, branch
                xchg    dx,bx
                lodsb                     ; read the byte from the buffer
                out     dx,al             ; send it
                xchg    bx,dx
                loop    wrnext            ; loop until all sent
                jmp     wrok

wrnot_ready:    cmp     di,timer          ; have we timed out ?
                jne     wrnext            ; no, branch

; Is the count the same as it was at the last timeout

                cmp     cx,last_count
                je      wrgiveup          ; Yes  -- exit

; Record the new count and reset the timeout

                mov     last_count,cx
                mov     ax,timer
                add     ax,timeout
                dec     ax
                mov     di,ax
                jmp     wrnext

wrgiveup:       mov     timeout_fl,2
                mov     ax,count
                sub     ax,cx
                jmp     wrterm

wrok:           mov     ax,count
wrterm:         ret

B004WriteLink   endp

                end
