Dim Arg
Set Arg = WScript.Arguments

Dim FSO,objReadFile,strLine,DestinationFile,BLName,BLHexFile,MergedResultContent
Set FSO=CreateObject("Scripting.FileSystemObject")

SourceFile = Arg(0)
BLHexFile = Arg(1)
OutputFileName = Arg(2)


'Merge the latest Bootloader(BL) file with the generated hex file and save in a merged file with filename of bootloader and program version

If FSO.FileExists(SourceFile) Then
	If FSO.FileExists(BLHexFile) Then
		
		Dim filecontent,objectFile
		Dim line
		Dim pos
		Dim addr
		
		Dim jumpaddr
		jumpaddr = "no init!"
		Dim BLjumpaddr
		BLjumpaddr = "no init!"
		
		Dim progcode 
		progcode = ""
		Dim bootcode
		bootcode = ""
		Dim Mergecode
		Mergecode = ""
		
			
		Set objectFile = FSO.OpenTextFile(SourceFile, 1, False)
		
		Set objectFile = FSO.OpenTextFile(BLHexFile, 1, False)
		
		Do until objectFile.AtEndOfStream
			line = objectFile.ReadLine
			
			If ReadIntelHexLineType(line) = 4 Then
				pos = ConvertHex2Dec(ReadIntelHexLineData(line),0)
				bootcode = bootcode & line & vbCrLf
			ElseIf ReadIntelHexLineType(line) = 0 Then
				addr = ReadIntelHexLineAddr(line)
				
				If pos = 0 Then
					If addr = 0 Then
						'Wscript.Echo "line" & line & "  " & ReadIntelHexLineData(line)
						'Wscript.Echo "line" & ReadIntelHexLineData(line)
						BLjumpaddr = Mid(ReadIntelHexLineData(line),1,8)
					ElseIf addr >= ConvertHex2Dec("6680",0) Then
						bootcode = bootcode & line & vbCrLf
					End If
				Else
					bootcode = bootcode & line & vbCrLf
				End If
			ElseIf ReadIntelHexLineType(line) = 1 Then
				bootcode = bootcode & line & vbCrLf
			End If
		Loop

		Set objectFile = FSO.OpenTextFile(SourceFile, 1, False)
		
		Do until objectFile.AtEndOfStream
			line = objectFile.ReadLine
			
			If ReadIntelHexLineType(line) = 4 Then
				pos = ConvertHex2Dec(ReadIntelHexLineData(line),0)
				progcode = progcode & line & vbCrLf
			ElseIf ReadIntelHexLineType(line) = 0 Then
				addr = ReadIntelHexLineAddr(line)
				If pos = 0 Then
					If addr = 0 Then
						jumpaddr = ReadIntelHexLineData(line)
						'Wscript.Echo "progcode :" & Mid(line,1,9) & BLjumpaddr & Mid(line,18)
						progcode = progcode & CovertCRC(Mid(line,1,9) & BLjumpaddr & Mid(line,18)) & vbCrLf
					Else
						progcode = progcode & line & vbCrLf
					End If
				End If
			End If
		Loop

		Mergecode = progcode & ":020000040000FA" & vbCrLf &  CovertCRC(":10667000FFFFFFFFFFFFFFFF"&jumpaddr&"FFFFFFFFAA") & vbCrLf & bootcode
		
		'Wscript.Echo "Save file as " + OutputFileName
		
		Set objOutFile = FSO.CreateTextFile(OutputFileName,True)
		objOutFile.Write(Mergecode)
		objOutFile.Close
	End If
End If

set objFSO = nothing
wscript.quit()


Function ReadIntelHexLineData(line)
	Dim length
	length = CInt(Mid(line,2,2))
	ReadIntelHexLineData = Mid(line,10,length*2)
End Function

Function ReadIntelHexLineAddr(line)
	ReadIntelHexLineAddr = ConvertHex2Dec(Mid(line,4,4),0)
End Function	
	
Function ConvertHex2Dec(hexx,print)
	If print = 1 Then
		'Wscript.Echo "ConvertHex2Dec'" & hexx & "'" 
	End If
	ConvertHex2Dec = CLng("&h" & hexx)
End Function

Function ReadIntelHexLineType(line)
	ReadIntelHexLineType = CInt(Mid(line,8,2))
End Function
	
Function CovertCRC(input)
	Dim i
	Dim count
	count = extract(input,2)
	Dim sum
	sum =count
	count = count +4
	'Wscript.Echo "input :" & input
	For i = 1 To count-1
		sum = sum + extract(input,i*2+2)
	Next
	'Wscript.Echo "sum :" & sum
	sum = sum Mod 256
	sum = 256-sum
	'Wscript.Echo "sum :" & decimalToHex(sum)
	CovertCRC = Mid(input,1,count*2+1) & decimalToHex(sum)
End Function
	
	
Function extract(str,y) 
	'Wscript.Echo "extraxt :" & str & "y" & y & "mid" & Mid(str,y,2)
	extract = ConvertHex2Dec(Mid(str,y,2),1)
End Function

Function decimalToHex(dec) 
	Dim result
	result = 0
	result = Hex(dec)
	If dec<16 Then
		decimalToHex = "0" & result
	Else
		decimalToHex = result
	End If
End Function
