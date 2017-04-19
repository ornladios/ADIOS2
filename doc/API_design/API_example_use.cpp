/* Fake example for Current API by design, incorrect as code but describes most
 * of the ideas */
#include <mpi.h>

#include "adios2/ADIOS.h"

void cb_AsyncWriteAdvanceCompleted(std::shared_ptr<adios::Engine> writer)
{
    std::cout << "AdvanceAsync() completed. We can modify our zero-copy "
                 "variables\n";
}

void cb_AsyncReadAdvanceCompleted(std::shared_ptr<adios::Engine> writer)
{
    std::cout
        << "AdvanceAsync() completed. We have new data to read and we have "
           "the lock on it\n";
}

int main(int argc, char *argv[])
{
    // Application variables
    MPI_Comm comm = MPI_COMM_WORLD;
    const unsigned int NX = 10;
    double Temperature[1][NX + 2]; // We will want to write only Nx elements,
                                   // skipping the first and last (think ghost
                                   // cells)
    std::vector<float> RaggedArray;

    int Nparts, nproc;

    // Global class/object that serves for init, finalize and provides ADIOS
    // functions
    adios::ADIOS adios("config.xml", comm, /*verbose=*/adios::Verbose::INFO,
                       /*debugflag=*/false);

    /*************
     * WRITE API
     *************/

    /* Method
     * We associate Engines and Transports and user settings into an object
     * called
     * Method.
     * ADIOS checks if it is defined by the user in the config file, and fills
     * it
     * out if it is.
     */

    adios::Method &wmethod = adios.DeclareMethod("WriteMethod");
    if (!wmethod.isUserDefined())
    {
        // if not defined by user, we can change the default settings
        wmethod.SetEngine("BP"); // BP is the default engine
        wmethod.AllowThreads(
            1); // no threading for data processing (except for staging)
        wmethod.AddTransport(
            "File", "lucky=yes"); // ISO-POSIX file is the default transport
        wmethod.AddTransport("Staging"); //"The" staging method developed in ECP
        wmethod.SetParameters("have_metadata_file",
                              "yes"); // Passing parameters to the engine
        wmethod.SetParameters("Aggregation",
                              (nproc + 1) / 2); // number of aggregators
        wmethod.SetParameters("verbose",
                              adios::Verbose::WARN); // Verbosity level
                                                     // for this engine
                                                     // and what it calls
    }

    // Define variables with transformations.
    adios::Variable<unsigned int> &varNX = adios.DefineVariable<unsigned int>(
        "NX"); // global single-value across processes
    auto &varNproc = adios.DefineVariable<int>(
        "nproc", adios::Dims{adios::GLOBAL_VALUE}); // same def for global value
    adios::Variable<int> &varNparts = adios.DefineVariable<int>(
        "Nparts", adios::Dims{adios::LOCAL_VALUE}); // a single-value
                                                    // different on every
                                                    // process
    adios::Variable<double> &var2D = adios.DefineVariable<double>(
        "Temperature",
        adios::Dims{nproc, NX}); // 2D global array, 1D decomposition
    adios::Variable<float> &varRagged = adios.DefineVariable<float>(
        "Ragged", adios::Dims{nproc, adios::VARYING_DIMENSION}); // ragged array

    // add transform to variable
    adios::Transform compress = adios::transform::BZip2();
    var2D.AddTransform(compress, "level=5");

    // open...write.write.write...advance...write.write.write...advance...
    // ...close  cycle
    // "w" create/overwrite on open, "a" append at open, "u" open for update
    // (does
    // not increase step), "r" open for read.
    std::shared_ptr<adios::Engine> writer = adios.Open(
        "myNumbers.bp", "w", comm, wmethod, adios::IOMode::INDEPENDENT);

    if (writer == nullptr)
        throw std::ios_base::failure("ERROR: failed to open ADIOS writer\n");

    // Zero-Copy API: Define a variable with local dimensions now, and make
    // ADIOS
    // allocate it inside its buffers
    // This requires an engine created. The variable will be deallocated in
    // writer->Close()
    // Calling varZeroCopy.SetSelection() later should throw an exception, i.e.
    // modification is not allowed
    // 2D global array, 1D decomposition
    adios::Variable<double> &varZeroCopy = adios.DefineVariable<double>(
        "ZC", adios::Dims{nproc, NX}, adios::Dims{1, NX}, adios::Dims{rank, 0});
    double fillValue = -1.0;
    double *const myVarZC =
        writer->AllocateVariable<double>(varZeroCopy, fillValue);

    for (int step = 0; step < 10; ++step)
    {
        // write scalar value
        writer->Write<int>(varNparts, Nparts);

        // Make a selection to describe the local dimensions of the variable we
        // write and
        // its offsets in the global spaces. This could have been done in
        // adios.DefineVariable()
        adios::SelectionBoundingBox sel = adios::SelectionBoundingBox(
            {1, NX}, {rank, NX}); // local dims and offsets; both as list
        var2D.SetSelection(
            sel); // Shall we call it SetSpaceSelection, SetOutputSelection?

        // Select the area that we want to write from the data pointer we pass
        // to
        // the writer
        // Think HDF5 memspace, just not hyperslabs yet, only a bounding box
        // selection
        // Engine will copy this bounding box from the data pointer into the
        // buffer.
        // Size of the bounding box should match the
        // "space" selection which was given above. Default memspace is the full
        // selection.
        adios::SelectionBoundingBox memspace = adios::SelectionBoundingBox(
            {1, NX}, {0, 1}); // local dims and offsets; both as list
        var2D.SetMemorySelection(memspace);

        writer->Write<double>(var2D, *Temperature);

        // Indicate we are done for this step.
        // N-to-M Aggregation, disk I/O will be performed during this call,
        // unless
        // time aggregation postpones all of that to some later step.
        // When Advance() returns, user can overwrite its Zero Copy variables.
        // Internal buffer is freed only if there are no Zero Copy variables and
        // there is no time aggregation going on
        writer->Advance();                   // same as AppendMode
        writer->Advance(adios::APPEND, 0.0); // append new step at next write
        writer->Advance(adios::UPDATE);      // do not increase step;

        // When AdvanceAsync returns, user need to wait for notification that he
        // can
        // overwrite the Zero Copy variables.
        writer->AdvanceAsync(adios::APPEND, cb_AsyncWriteAdvanceCompleted);
    }

    // Called once: indicate that we are done with this output for the run
    // Zero Copy variables will be deallocated
    writer->Close();

    /*************
     * READ API
     *************/
    adios::Method &rmethod = adios.DeclareMethod("ReadMethod");
    if (!rmethod.isUserDefined())
    {
        // if not defined by user, we can change the default settings
        rmethod.SetEngine("BP");         // BP is the default engine
        rmethod.AddTransport("Staging"); //"The" staging method developed in ECP
        rmethod.SetParameters("Aggregation",
                              (nproc + 1) / 2); // number of aggregators
        rmethod.SetParameters("verbose",
                              adios::Verbose::WARN); // Verbosity level
                                                     // for this engine
                                                     // and what it calls
    }

    // 1. Open a stream, where every reader can see everything in a stream (so
    // that they can read a global array)
    // Blocking read of a variable
    try
    {
        // These method settings are developer-only, not available to the user
        rmethod.SetReadMultiplexPattern(adios::GLOBAL_READERS); // Each reader
                                                                // process sees
        // everything from
        // the stream
        rmethod.SetStreamOpenMode(
            adios::WAITFORSTREAM); // In Open(), wait for the
                                   // first step appear
                                   // (default)

        // Open a stream
        std::shared_ptr<adios::Engine> reader = adios.Open(
            "filename.bp", "r", comm, rmethod, adios::IOMode::COLLECTIVE,
            /*timeout_sec=*/300.0); // wait this long for the stream,
                                    // return error afterwards

        /* Variable names are available as a vector of strings */
        std::cout << "List of variables in file: " << reader->VariableNames()
                  << "\n";
        /* read a Global scalar which has a single value in a step */
        reader->Read<unsigned int>("NX", NX);

        // inquiry about a variable, whose name we know
        adios::Variable<double> var2D =
            reader->InquireVariableDouble("Temperature");
        std::vector<std::size_t> gdims = var2D.GetGlobalDimensions();
        int nsteps = var2D.GetSteps();

        struct adios::BlockInfo blocks = reader.InquiryVariableBlockInfo(
            reader, var2D); // get per-writer size info
        // this is adios1 ADIOS_VARBLOCK
        struct adios::Statistics stats =
            reader.InquiryVariableStat(reader, var2D, perstepstat,
                                       perblockstat); // get min/max statistics
        // this is adios1 ADIOS_VARSTAT

        while (true)
        {
            // Make a selection to describe the local dimensions of the variable
            // we
            // READ and
            // its offsets in the global spaces
            adios::SelectionBoundingBox bbsel = adios::SelectionBoundingBox(
                {1, NX}, {0, 0}); // local dims and offsets; both as list
            var2D.SetSelection(bbsel);
            adios::SelectionBoundingBox memspace = adios::SelectionBoundingBox(
                {1, NX}, {0, 1}); // local dims and offsets; both as list
            var2D.SetMemorySelection(memspace);
            reader->Read<double>(var2D, *Temperature);
            // var2D, Temperature );

            // Better for staging to schedule several reads at once
            reader->ScheduleRead<double>(var2D, *Temperature);
            reader->PerformReads(adios::BLOCKINGREAD);

            // promise to not read more from this step/item
            reader->Release();

            // want to move on to the next available step/item
            reader->Advance(adios::NEXT_AVAILABLE);   // default
            reader->Advance(adios::LATEST_AVAILABLE); // interested only in the
                                                      // latest data
        }
        // Close file/stream
        reader->Close();
    }
    catch (adios::end_of_stream &e)
    {
        //  Reached end of stream, end processing loop
        // Close file/stream
        reader->Close();
    }
    catch (adios::file_not_found &e)
    {
        // File/stream does not exist, quit
    }

    // 2. Open a stream, where each item from the writers will get to a single
    // reader only
    // If the writers are collective, that means a whole steps go to different
    // readers
    // If the writers are independent, that means each writer's output goes to
    // different readers
    // Also show here ScheduleRead/PerformRead
    // try
    {
        rmethod.SetReadMultiplexPattern(
            adios::FIFO_READERS); // Each reader process
                                  // sees everything
                                  // from the stream
        rmethod.SetStreamOpenMode(
            adios::WAITFORSTREAM); // In Open(), wait for the
                                   // first step appear
                                   // (default)

        // Open a stream
        std::shared_ptr<adios::Engine> reader = adios.Open(
            "filename.bp", "r", comm, rmethod, adios::IOMode::INDEPENDENT,
            /*timeout_sec=*/300.0); // wait this long for the stream, return
                                    // error
                                    // afterwards

        while (true)
        {
            // Make a selection to describe the local dimensions of the variable
            // we
            // READ and
            // its offsets in the global spaces if we know this somehow
            adios::SelectionBoundingBox bbsel = adios::SelectionBoundingBox(
                {1, NX}, {0, 0}); // local dims and offsets; both as list
            var2D.SetSelection(bbsel);
            reader->Read<double>(var2D, *Temperature);

            // Let ADIOS allocate space for the incoming (per-writer) item
            double *data = reader->Read<double>(var2D);

            // promise to not read more from this step/item
            reader->Release();

            // want to move on to the next available step/item
            reader->Advance(); // default
            reader->Advance(
                adios::LATEST_AVAILABLE); // This should be an error, or
                                          // could it make sense?
        }
        reader->Close();
    }

    // 3. Open a stream and return immediately, not waiting for data to appear
    // In this mode we cannot inquiry variables, but can schedule reads
    // try
    {
        rmethod.SetReadMultiplexPattern(adios::GLOBAL_READERS); // Each reader
                                                                // process sees
        // everything from
        // the stream
        rmethod.SetStreamOpenMode(
            adios::NOWAITFORSTREAM); // In Open(), wait for
                                     // the first step appear
                                     // (default)

        // Open a stream
        std::shared_ptr<adios::Engine> reader = adios.Open(
            "filename.bp", "r", comm, rmethod, adios::IOMode::INDEPENDENT);

        while (true)
        {

            // Let ADIOS allocate space for the incoming (per-writer) item
            reader->ScheduleRead(); // read whatever comes

            // One way is to handle the incoming data through a callback (which
            // will
            // be called in a thread)
            // void cb( const void *data, std::string doid, std::string var,
            // std::string dtype, std::vector<std::size_t> varshape );
            // void cb( adios::VARCHUNK * chunk ); // see adios1 for VARCHUNK
            reader->SetReadCallback(cb);
            reader->PerformReads(adios::NONBLOCKINGREAD);

            // Another way is checking back manually like in adios1 and
            // processing
            // chunks
            reader->PerformReads(adios::NONBLOCKINGREAD);
            int ck;
            adios::VARCHUNK *chunk;
            try
            {
                while ((ck = reader->CheckReads(&chunk)) > 0)
                {
                    if (chunk)
                    {
                        // process the chunk first
                        // ...
                        // free memory of chunk (not the data!)
                        adios::FreeChunk(chunk);
                    }
                    else
                    {
                        // no chunk was returned, slow down a little
                        sleep(1);
                    }
                }
            }
            catch (std::exception &e)
            {
                // some error happened while getting a chunk
            }

            reader->Release();

            // When AdvanceAsync returns new data is not yet available.
            // A callback will tell us when we have the new data (and we have
            // the lock
            // on it to read)
            writer->AdvanceAsync(adios::NEXT_AVAILABLE,
                                 cb_AsyncReadAdvanceCompleted);
            // Do we need more fine grained control? Like this function does not
            // get
            // the lock and so
            // do we need to call Advance() to get the lock?
        }
        reader->Close();
    }
    // Note: chunk reading also works if scheduling reads for specific variables

    // 4. Open it as file and see all steps at once.
    // Allow for reading multiple steps of a variable into a contiguous array
    try
    {
        rmethod.AddTransport("BP"); // Set a file engine here. Shall we have
                                    // RemoveTransport() too?

        // Open a file with all steps immediately available
        std::shared_ptr<adios::Engine> reader = adios.OpenFileReader(
            "filename.bp", comm, rmethod, adios::IOMode::COLLECTIVE);

        /* NX */
        /* There is a single value for each step. We can read all into a 1D
         * array
         * with a step selection.
         * Steps are not automatically presented as an array dimension and read
         * does
         * not read it as array.
         */
        // We can also just conveniently get the first step with a simple read
        // statement.
        reader->Read<unsigned int>(
            "NX",
            &NX); // read a Global scalar which has a single value in a step
        reader->Read<unsigned int>(
            varNX,
            &NX); // read a Global scalar which has a single value in a step

        adios::Variable<void> varNXread = reader->InquireVariable("NX");
        adios::Variable<unsigned int> varNXreadint =
            reader->InquireVariableInt("NX");
        std::vector<unsigned int> Nxs(
            varNXread.GetSteps()); // number of steps available
        // make a StepSelection to select multiple steps. Args: From, #of
        // consecutive steps
        adios::StepSelection stepsNx(0, varNXread.GetSteps());
        // ? How do we make a selection for an arbitrary list of steps ?
        varNXread.SetStepSelection(stepsNx);
        reader->Read<unsigned int>(varNXreadint, Nxs.data());
        reader->Read<void>(varNXread, Nxs.data());

        auto itmax = std::max_element(std::begin(Nxs), std::end(Nxs));
        auto itmin = std::min_element(std::begin(Nxs), std::end(Nxs));
        if (*itmin != *itmax)
        {
            throw std::ios_base::failure(
                "ERROR: NX is not the same at all steps!\n");
        }

        /* Nparts */
        // Nparts local scalar is presented as a 1D array of nproc elements.
        // We can read all steps into a 2D array of nproc * nsteps
        adios::Variable<void> varNpartsread = reader->InquireVariable("Nparts");
        std::vector<int> partsV(Nproc * varNpartsread->GetSteps());
        varNpartsread->SetStepSelection(
            adios.StepSelection(0, varNpartsread.GetSteps()));
        reader->Read<int>("Nparts", partsV.data());
        reader->Read<void>(varNpartsread,
                           partsV.data()); // missing spatial selection = whole
                                           // array at each step

        // Close file/stream
        reader->Close();
    }
    catch (adios::file_not_found &e)
    {
        // File/stream does not exist, quit
    }

    return 0;
}
