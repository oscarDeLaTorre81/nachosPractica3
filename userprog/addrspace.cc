// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"
#include<iostream> 
#include<iterator> // for iterators 
#include<vector> // for vectors 
#include "queue" 
#include <string>
using namespace std; 
//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

static void 
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable, char *nombrePrograma, char *arg)
{
    char *buffer,*datos, *codigo;
	//printf("Argumento: %s\n", stats->xtraArg);
    NoffHeader noffH;
    unsigned int i, size;
	numPhys = 0;
	if(strcmp(stats->xtraArg,"-C") == 0 || strcmp(stats->xtraArg,"-M") == 0)
	{
		//printf("HIIIIIII %s", stats->algoritmo);
		if(strcmp(stats->algoritmo,"-F")==0){
			printf("Algoritmo FIFO (First In First Out)\n");
		}
		else if(strcmp(stats->algoritmo,"-L")==0){
			printf("Algoritmo LRU (Less Recently Used/Menos Recientemente Utilizado)\n");
		}
	}

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
		(WordToHost(noffH.noffMagic) == NOFFMAGIC))
    	SwapHeader(&noffH);
    ASSERT(noffH.noffMagic == NOFFMAGIC);

  nombreArchivoIntercambio=strcat(nombrePrograma,".swp");


 /* Mandamos llamar los metodos y el parametro nos devuelve la ubicacon,nombre y sector del archivo 
para poderlo abrir*/

 archivoIntercambio = fileSystem->Open( nombrePrograma );
 //Paso de lode segmentos: 

/*Se empieza a recupera el codigo del programa al swp*/
 codigo = new char[ noffH.code.size ];
 //printf("\nCodigo: %s \n", codigo );
 executable->ReadAt( codigo , noffH.code.size, noffH.code.inFileAddr);
 archivoIntercambio->WriteAt( codigo , noffH.code.size,0);

// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
	if(strcmp(stats->xtraArg,"-C") == 0)
		{	
			printf("Tamaño del proceso: %d bytes\n", size);
		}
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;
	if(strcmp(stats->xtraArg,"-C") == 0)	
	{		
		printf("Numero de páginas: %d\n\n", numPages);
	}

    //ASSERT(numPages <= NumPhysPages);		// check we're not trying
						// to run anything too big --
						// at least until we have
						// virtual memory

    DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
					numPages, size);
// first, set up the translation 

	/*Se recuperan los datos*/
 datos = new char[ noffH.initData.size ];
 executable->ReadAt( datos , noffH.initData.size , noffH.initData.inFileAddr );
 archivoIntercambio->WriteAt( datos , noffH.initData.size , noffH.code.size );
  
  
    
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;
    
    	

    pageTable = new TranslationEntry[numPages];

    for (i = 0; i < numPages; i++) {
	pageTable[i].virtualPage = i;	// for now, virtual page # = phys page #
	pageTable[i].physicalPage = i;
	pageTable[i].valid = FALSE;
	pageTable[i].use = FALSE;
	pageTable[i].dirty = FALSE;
	pageTable[i].age = 0;
	pageTable[i].readOnly = FALSE;  // if the code segment was entirely on 
					// a separate page, we could set its 
					// pages to be read-only
/*		//Imprime el indice
	printf("  %d    ",i);
	//imprime el numero de marco
	printf("   %d    \t",pageTable[i].virtualPage);
        //imprime el bit de validez
	printf("    %d   ",pageTable[i].valid);
	printf("\n");*/
    }
    
	if(strcmp(stats->xtraArg,"-M") == 0)
	{
		printf("Mapeo de direcciones logicas a fisica \n");
		printf("DirLogica		N.Pagina		Desplazamiento		DirFisica \n");  
	}
	else if(strcmp(stats->xtraArg,"-C") == 0)
	{	
		printf("VPN:\n");
	}  

// zero out the entire address space, to zero the unitialized data segment 
// and the stack segment
    bzero(machine->mainMemory, MemorySize);
	//memset(machine->mainMemory, 0x0, size);


// then, copy in the code and data segments into memory
    if (noffH.code.size > 0) {
        DEBUG('a', "Initializing code segment, at 0x%x, size %d\n", noffH.code.virtualAddr, noffH.code.size);
        //executable->ReadAt(&(machine->mainMemory[noffH.code.virtualAddr]),
	//		noffH.code.size, noffH.code.inFileAddr);
		executable->ReadAt(buffer, noffH.code.size, noffH.code.inFileAddr);
    }
    if (noffH.initData.size > 0) {
        DEBUG('a', "Initializing data segment, at 0x%x, size %d\n", noffH.initData.virtualAddr, noffH.initData.size);
        //executable->ReadAt(&(machine->mainMemory[noffH.initData.virtualAddr]),
	//		noffH.initData.size, noffH.initData.inFileAddr);
    	executable->ReadAt(buffer + noffH.code.size, noffH.initData.size, noffH.code.inFileAddr);
    }
	//Escribir el archivo de intercambio.
    archivoIntercambio->WriteAt(buffer, noffH.code.size + noffH.initData.size, 0);
}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
   delete pageTable;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}

void AddrSpace::pageSwap(int vpn,int dirLog)
{
	//printf("ADDRSPACE \n");
	//Calcular la dirección fisica.
	int  dirPhys = numPhys*PageSize;
	int page = vpn*PageSize;
	int victim = 0;
	int tempPhys = 0;
	//printf("Por aquí");
	//Leer el contenido del bloque y ponerlo en memoria.
	if(numPhys < NumPhysPages)
	{
		archivoIntercambio->ReadAt(&(machine->mainMemory[dirPhys]),PageSize, page);
		pageTable[vpn].physicalPage=numPhys;
		numPhys++;//Aumentar el número del marco.
	}
	else
	{
		if(strcmp(stats->algoritmo,"-L") == 0){
		victim = findRose();//Escoger una victima.
		pageTable[victim].age = 0;
	}
	else if(strcmp(stats->algoritmo,"-F")==0){
		victim = fifoVictim();
	}
		tempPhys = pageTable[victim].physicalPage;//Almacenar el número de la dirección fisica en memoria.
		dirPhys = tempPhys*PageSize;
		if(pageTable[victim].dirty)
		{	
			//printf("Estaba sucio\n");
			archivoIntercambio->WriteAt(&(machine->mainMemory[dirPhys]),PageSize, victim*PageSize);//Escribir las modificaciones.
			stats->numDiskWrites++;//Aumentar la variable de escritura.
			pageTable[victim].dirty = FALSE;//Poner el bit de modificación en falso.
		}
		
		pageTable[victim].valid = FALSE;//Invalidar el bit de validez.
		archivoIntercambio->ReadAt(&(machine->mainMemory[dirPhys]),PageSize, page);//Escribir el contenido del archivo de intercambio.
		pageTable[vpn].physicalPage = tempPhys;//Modificar la dirección fisica.
	}
	if(strcmp(stats->algoritmo,"-F") == 0){
		stats->fifoQueue.push(vpn);
		//pageTable[vpn].valid = true;
		//printf("Se agrego %d a la cola \n", vpn);
	}
	pageTable[vpn].valid=TRUE;
	stats->numDiskReads++;//Aumentar la variable de lectura.
	//Asignar el marco (dirección fisica de la página cargada).
	
}

bool AddrSpace:: findCharacter(std::vector<int> &inSearch, int toSearch)
{
	
	bool result = false;
	int help = 0;	
	//printf("inSearch = %d\n",(&inSearch)->back());
	for(vector<int>::iterator it = inSearch.end(); it > inSearch.begin(); it--)
	{
		//printf("i = %d , toSearch=%d\n",it,toSearch);
		help = *it;
		if( help == toSearch)
		{
			result = true;
		}	
	}
	return result;
}

//Función para encontrar la victima
int AddrSpace::findRose()
{
	int victim = 0;
	int temp = 0;
	//std::vector<int> helper;//En este arreglo se almacenarán en orden los elementos según su primera aparición en la cadena de referencía
	//helper.push_back(stats->vpnRefArray.back());
	//bool result = false;
	//printf("\nHola\n");
	/*for(int i = stats->vpnRefArray.size()-1;  i>=0; i--)
	{
		temp = stats->vpnRefArray[i];
		
		//Buscar que no se haya agregado el número de pagina previamente
		for(int j = helper.size() && !result;  j>=0; j--)
		{
			printf("j = %d [j] = %d [i] = %d\n",j,helper[j],temp);
			if( helper[j] == temp)
			{
				result = true;
			}	
		}

		if(!result)	//if(findCharacter(helper,stats->vpnRefArray[i]) == false) 
		{
			
			printf("No estaba: %d\n",temp);
			helper.push_back(temp);//Agregar el elemento dentro de la lista de ayuda.
			printf("Size = %d\n",helper.size());
			
			for(int j = helper.size() -1; j>=0; j--)
			{
				printf("j = %d ,[j] = %d\n",j,helper[j]);	
			}
		}
			result = false;
			//printf("No fue\n");
		
		
	}*/
	//printf("Salió\n");
	for(int i = 0; i<numPages;i++)
	{
		if(pageTable[i].valid && pageTable[i].age>=pageTable[victim].age)
		{
			victim = i;
		}
		/*if(pageTable[i].valid)
		{
			printf("Page = %d age = %d\n",i,pageTable[i].age);
		}*/
	}	
	//victim = helper.back();//Retornar el ultimo caracter agregado a la lista.
	printf("Victima = %d\n",victim);
	return victim;
}
int AddrSpace::fifoVictim(){
	int victim = stats->fifoQueue.front(); //regresa el primer valor de la cola
	//pageTable[victim].valid = false; // cambia el bit valid a falso
	stats->fifoQueue.pop(); //saca el primer elemento
	return victim;
}