/* Current API by design, incorrect as code but describes all the ideas */

#include "ADIOS.h"


namespace adios
{
typedef enum {
    VARYING_DIMENSION = -1,
    LOCAL_VALUE = 0,
    GLOBAL_VALUE = 1
};

typedef enum { ERROR = 0, WARN = 1, INFO = 2, DEBUG = 3 } VerboseFlag;

typedef enum {
    COLLECTIVE_WRITERS = 0, INDEPENDENT_WRITERS = 1,
    GLOBAL_READERS = 2, ROUNDROBIN_READERS = 3, FIFO_READERS = 4,
    OPEN_ALL_STEPS = 5
} OpenMode;

typedef enum { NOWAITFORSTREAM = 0, WAITFORSTREAM = 1 } StreamOpenMode; // default: wait for stream

typedef enum {
    APPEND = 0, UPDATE = 1,                    // writer advance modes
    NEXT_AVAILABLE = 2, LATEST_AVAILABLE = 3,  // reader advance modes
} AdvanceMode;

typedef enum { NONBLOCKINGREAD = 0, BLOCKINGREAD = 1 } ReadMode;

} // namespace adios

void dummy()
{
    //Application variables
    std::vector<double> Temperature;
    std::vector<float> RaggedArray;
    unsigned int Nx;
    int Nparts, nproc;

    // Global class/object that serves for init, finalize and provides ADIOS functions
    adios::ADIOS adios( std::string configfile, MPI_Comm comm, adios::verboseflag flag, bool debugflag );


    /*************
     * WRITE API
     *************/

    /* Method
     * We associate Engines and Transports and user settings into an object called Method.
     * ADIOS check if it is defined by the user in the config file, and fills it out if it is.
     */

    std::shared_ptr<adios::Method> method = adios.DeclareMethod( "MethodName" );
    if( ! method.isDefinedInConfig() )
    {
        // if not defined by user, we can change the default settings
        method.SetEngine( "BP" ); // BP is the default engine
        method.AddTransport( "File", "lucky=yes" ); // ISO-POSIX file is the default transport
        method.AddTransport( "Staging" ); //"The" staging method developed in ECP
        method.SetParameters("have_metadata_file","yes" ); // Passing parameters to the engine
        method.SetParameters( "Aggregation", (nproc+1)/2 ); // number of aggregators
        method.SetParameters( "verbose", adios::WARN ); // Verbosity level for this engine and what it calls
    }


    //Define variables with transformations.
    adios::Variable<unsigned int>& varNX = adios.DefineVariable<unsigned int>( "NX" ); // global single-value across processes
    adios::Variable<int>&    varNproc   = adios.DefineVariable<int>( "nproc", adios::GLOBAL_VALUE ); // same def for global value
    adios::Variable<int>&    varNparts  = adios.DefineVariable<int>( "Nparts", adios::LOCAL_VALUE ); // a single-value different on every process
    adios::Variable<double>& var1D      = adios.DefineVariable<double>( "Temperature", {nproc*Nx} ); // 1D global array
    adios::Variable<float>&  varRagged  = adios.DefineVariable<float>( "Ragged", {nproc,adios::VARYING_DIMENSION} ); // ragged array

    // Define a variable with local dimensions now, and make ADIOS allocate it inside its buffers (zero-copy API)
    adios::Variable<double>& varZeroCopy = adios.DefineVariable<double>( "ZC", {nproc,Nx}, {1,NX}, {rank,0} ); // 2D global array, 1D decomposition
    double *myVarZC = adios.AllocateVar( varZeroCopy );

    //add transform to variable
    adios::Transform bzip2 = adios::transform::BZIP2( );
    var1D->AddTransform( bzip2, 1 );

    // open...write.write.write...advance...write.write.write...advance... ...close  cycle
    // "w" create/overwrite on open, "a" append at open, "u" open for update (does not increase step), "r" open for read.
    std::shared_ptr<adios::Engine> writer = adios.Open( "myNumbers.bp", "w", method, adios::INDEPENDENT_WRITERS );
    if( writer == nullptr )
        throw std::ios_base::failure( "ERROR: failed to open ADIOS writer\n" );

    for (int step = 0; step < 10; ++step) {
        // write scalar value
        writer->Write<int>( varNparts, Nparts );

        // Make a selection to describe the local dimensions of the variable we write and
        // its offsets in the global spaces
        adios::Selection& sel = adios.SelectionBoundingBox( {Nx}, {rank*Nx} ); // local dims and offsets; both as list
        var1D.SetSelection( sel );
        writer->Write<double>( var1D, Temperature.data() );

        // Indicate we are done for this step.
        // N-to-M Aggregation, disk I/O will be performed during this call, unless
        // time aggregation postpones all of that to some later step.
        // When Advance() returns, user can overwrite its Zero Copy variables.
        // Internal buffer is freed only if there are no Zero Copy variables and there is no time aggregation going on
        writer->Advance(); // same as AppendMode
        writer->Advance( adios::APPEND ); // append new step at next write
        writer->Advance( adios::UPDATE ); // do not increase step;  ? should this cause error in staging ?

        // When AdvanceAsync returns, user need to wait for notification that he can overwrite the Zero Copy variables.
        writer->AdvanceAsync( callback_func_to_notify_me() );
    }

    // Called once: indicate that we are done with this output for the run
    // Zero Copy variables will be deallocated
    writer->Close();



    /*************
     * READ API
     *************/

    // 1. Open a stream, where every reader can see everything in a stream (so that they can read a global array)
    // Blocking read of a variable
    try
    {

        // Open a stream
        std::shared_ptr<adios::Engine> reader =
                adios.Open( "filename.bp", "r", method,
                        adios::GLOBAL_READERS, // Each reader process sees everything from the stream
                        adios::WAITFORSTREAM, // wait for the first step appear (default)
                        timeout ); // wait this long for the stream, return error afterwards

        /* Variable names are available as a vector of strings */
        std::cout << "List of variables in file: " << reader->VariableNames << "\n";
        /* read a Global scalar which has a single value in a step */
        reader->Read<unsigned int>( "NX", Nx );

        // inquiry about a variable, whose name we know
        adios::Variable var1D = reader.InquiryVariable( "Temperature" );
        vector<uint64_t> gdims = var1D->GetGlobalDimensions();
        int step = varID->GetStep();

        struct adios::BlockInfo blocks = reader.InquiryVariableBlockInfo( reader, var1D ); // get per-writer size info
        // this is adios1 ADIOS_VARBLOCK
        struct adios::Statistics stats = reader.InquiryVariableStat( reader, var1D, perstepstat, perblockstat ); // get min/max statistics
        // this is adios1 ADIOS_VARSTAT

        while( true )
        {
            // Make a selection to describe the local dimensions of the variable we READ and
            // its offsets in the global spaces
            adios::Selection bbsel = adios.SelectionBoundingBox( {ldim}, {offs} ); // local dims and offsets; both as list
            var1D->SetSelection( bbsel );
            reader->Read<double>( var1D, Temperature.data() );

            // Better for staging to schedule several reads at once
            reader->ScheduleRead<double>( var1D, Temperature.data() );
            reader->PerformRead( adios::BLOCKINGREAD );

            // promise to not read more from this step/item
            reader->Release();

            // want to move on to the next available step/item
            reader->Advance(adios::NEXT_AVAILABLE);   // default
            reader->Advance(adios::LATEST_AVAILABLE); // interested only in the latest data
        }
        // Close file/stream
        reader->Close();
    }
    catch( adios::end_of_stream& e )
    {
        //  Reached end of stream, end processing loop
        // Close file/stream
        bpReader->Close();
    }
    catch( adios::file_not_found& e )
    {
        // File/stream does not exist, quit
    }



    // 2. Open a stream, where each item from the writers will get to a single reader only
    // If the writers are collective, that means a whole steps go to different readers
    // If the writers are independent, that means each writer's output goes to different readers
    // Also show here ScheduleRead/PerformRead
    //try
    {

        // Open a stream
        std::shared_ptr<adios::Engine> reader =
                adios.Open( "filename.bp", "r", method,
                        adios::FIFO_READERS, // Each reader process sees everything from the stream
                        adios::WAITFORSTREAM, // wait for the first step appear (default)
                        timeout ); // wait this long for the stream, return error afterwards

        while( true )
        {
            // Make a selection to describe the local dimensions of the variable we READ and
            // its offsets in the global spaces if we know this somehow
            adios::Selection bbsel = adios.SelectionBoundingBox( {ldim}, {offs} ); // local dims and offsets; both as list
            var1D->SetSelection( bbsel );
            reader->Read<double>( var1D, Temperature.data() );

            // Let ADIOS allocate space for the incoming (per-writer) item
            double * data = reader->Read<double>( var1D );

            // promise to not read more from this step/item
            reader->Release();

            // want to move on to the next available step/item
            reader->Advance();   // default
            reader->Advance(adios::LATEST_AVAILABLE); // This should be an error, or could it make sense?
        }
        reader->Close();
    }


    // 3. Open a stream and return immediately, not waiting for data to appear
    // In this mode we cannot inquiry variables, but can schedule reads
    //try
    {

        // Open a stream
        std::shared_ptr<adios::Engine> reader =
                adios.Open( "filename.bp", "r", method,
                        adios::GLOBAL_READERS, // Each reader process sees everything from the stream
                        adios::NOWAITFORSTREAM // wait for the first step appear (default)
                        );

        while( true )
        {

            // Let ADIOS allocate space for the incoming (per-writer) item
            reader->ScheduleRead<void>();  // read whatever comes

            // One way is to handle the incoming data through a callback (which will be called in a thread)
            // void cb( const void *data, std::string doid, std::string var, std::string dtype, std::vector<std::size_t> varshape );
            // void cb( adios::VARCHUNK * chunk ); // see adios1 for VARCHUNK
            reader->SetReadCallback( cb );
            reader->PerformRead( adios::NONBLOCKINGREAD );

            // Another way is checking back manually like in adios1 and processing chunks
            reader->PerformRead( adios::NONBLOCKINGREAD );
            int ck;
            adios::VARCHUNK * chunk;
            try
            {
                while ( (ck = reader->CheckReads( &chunk )) > 0) {
                    if (chunk) {
                        // process the chunk first
                        // ...
                        // free memory of chunk (not the data!)
                        adios::FreeChunk(chunk);
                    } else {
                        // no chunk was returned, slow down a little
                        sleep(1);
                    }
                }
            }
            catch( std::exception& e )
            {
                // some error happened while getting a chunk
            }

            reader->Release();
            reader->Advance();
        }
        reader->Close();
    }


    // 4. Open it as file and see all steps at once.
    // Allow for reading multiple steps of a variable into a contiguous array
    try
    {

        // Open a stream
        std::shared_ptr<adios::Engine> reader =
                adios.Open( "filename.bp", "r", method,
                        adios::OPEN_ALL_STEPS, //File mode, no advance
                        );

        /* NX */
        /* There is a single value for each step. We can read all into a 1D array with a step selection.
         * Steps are not automatically presented as an array dimension and read does not read it as array.
         */
        // We can also just conveniently get the first step with a simple read statement.
        reader->Read<unsigned int>( "NX", &Nx );  // read a Global scalar which has a single value in a step


        adios::Variable<void> varNx = bpReader.InquiryVariable("Nx");
        std::vector<int> Nxs( varNx->GetSteps() );  // number of steps available
        // make a StepSelection to select multiple steps. Args: From, #of consecutive steps
        adios::StepSelection stepsNx( 0, varNx->GetSteps() );
        // ? How do we make a selection for an arbitrary list of steps ?
        varNX.SetStepSelection( stepsNx );
        reader->Read<unsigned int>( varNx, Nxs.data() );

        auto itmax = std::max_element(std::begin(Nxs), std::end(Nxs));
        auto itmin = std::min_element(std::begin(Nxs), std::end(Nxs));
        if (*itmin != *itmax)
        {
            throw std::ios_base::failure( "ERROR: NX is not the same at all steps!\n" );
        }


        /* Nparts */
        // Nparts local scalar is presented as a 1D array of nproc elements.
        // We can read all steps into a 2D array of nproc * nsteps
        adios::Variable<void> varNparts = bpReader.InquiryVariable("Nparts");
        std::vector<int> partsV( Nproc * varNparts->GetSteps() );
        varNparts->SetStepSelection(
                                     adios.StepSelection( 0, varNparts->GetSteps() )
                                   );
        bpReader->Read<int>( varNparts, partsV.data() ); // missing spatial selection = whole array at each step

        // Close file/stream
        reader->Close();
    }
    catch( adios::end_of_stream& e )
    {
        //  Reached end of stream, end processing loop
        // Close file/stream
        bpReader->Close();
    }
    catch( adios::file_not_found& e )
    {
        // File/stream does not exist, quit
    }

}
