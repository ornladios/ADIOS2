/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Analysis code for the Gray-Scott simulation.
 * Computes mean curvature at each point on an isosurface.
 *
 * Keichi Takahashi <keichi@is.naist.jp>
 *
 */

#include <fstream>
#include <iostream>

#include <adios2.h>

#include <vtkCellArray.h>
#include <vtkCurvatures.h>
#include <vtkDoubleArray.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>

#include "../common/timer.hpp"

vtkSmartPointer<vtkPolyData> read_mesh(const std::vector<double> &bufPoints,
                                       const std::vector<int> &bufCells,
                                       const std::vector<double> &bufNormals)
{
    int nPoints = static_cast<int>(bufPoints.size() / 3);
    int nCells = static_cast<int>(bufCells.size() / 3);

    auto points = vtkSmartPointer<vtkPoints>::New();
    points->SetNumberOfPoints(nPoints);
    for (vtkIdType i = 0; i < nPoints; i++)
    {
        points->SetPoint(i, &bufPoints[i * 3]);
    }

    auto polys = vtkSmartPointer<vtkCellArray>::New();
    for (vtkIdType i = 0; i < nCells; i++)
    {
        vtkIdType a = bufCells[i * 3 + 0];
        vtkIdType b = bufCells[i * 3 + 1];
        vtkIdType c = bufCells[i * 3 + 2];

        polys->InsertNextCell(3);
        polys->InsertCellPoint(a);
        polys->InsertCellPoint(b);
        polys->InsertCellPoint(c);
    }

    auto normals = vtkSmartPointer<vtkDoubleArray>::New();
    normals->SetNumberOfComponents(3);
    for (vtkIdType i = 0; i < nPoints; i++)
    {
        normals->InsertNextTuple(&bufNormals[i * 3]);
    }

    auto polyData = vtkSmartPointer<vtkPolyData>::New();
    polyData->SetPoints(points);
    polyData->SetPolys(polys);
    polyData->GetPointData()->SetNormals(normals);

    return polyData;
}

void compute_curvature(const vtkSmartPointer<vtkPolyData> polyData)
{
    vtkSmartPointer<vtkCurvatures> curvaturesFilter = vtkSmartPointer<vtkCurvatures>::New();
    curvaturesFilter->SetInputData(polyData);
    // curvaturesFilter->SetCurvatureTypeToMinimum();
    // curvaturesFilter->SetCurvatureTypeToMaximum();
    // curvaturesFilter->SetCurvatureTypeToGaussian();
    curvaturesFilter->SetCurvatureTypeToMean();
    curvaturesFilter->Update();
}

int main(int argc, char *argv[])
{
    int provided;
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided);

    int rank, procs, wrank;

    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);

    const unsigned int color = 7;
    MPI_Comm comm;
    MPI_Comm_split(MPI_COMM_WORLD, color, wrank, &comm);

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &procs);

    if (argc < 3)
    {
        if (rank == 0)
        {
            std::cerr << "Too few arguments" << std::endl;
            std::cout << "Usage: curvature input output" << std::endl;
        }
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    const std::string input_fname(argv[1]);
    const std::string output_fname(argv[2]);

    adios2::ADIOS adios("adios2.xml", comm);

    adios2::IO inIO = adios.DeclareIO("IsosurfaceOutput");
    adios2::Engine reader = inIO.Open(input_fname, adios2::Mode::Read);

    adios2::IO outIO = adios.DeclareIO("CurvatureOutput");
    adios2::Engine writer = outIO.Open(output_fname, adios2::Mode::Write);

    std::vector<double> points;
    std::vector<int> cells;
    std::vector<double> normals;
    int step;

#ifdef ENABLE_TIMERS
    Timer timer_total;
    Timer timer_read;
    Timer timer_compute;

    std::ostringstream log_fname;
    log_fname << "compute_curvature_pe_" << rank << ".log";

    std::ofstream log(log_fname.str());
    log << "step\ttotal_curv\tread_curv\tcompute_curv" << std::endl;
#endif

    while (true)
    {
#ifdef ENABLE_TIMERS
        MPI_Barrier(comm);
        timer_total.start();
        timer_read.start();
#endif

        adios2::StepStatus status = reader.BeginStep();

        if (status != adios2::StepStatus::OK)
        {
            break;
        }

        auto varPoint = inIO.InquireVariable<double>("point");
        auto varCell = inIO.InquireVariable<int>("cell");
        auto varNormal = inIO.InquireVariable<double>("normal");
        auto varStep = inIO.InquireVariable<int>("step");

        if (varPoint.Shape().size() > 0 || varCell.Shape().size() > 0)
        {
            varPoint.SetSelection({{0, 0}, {varPoint.Shape()[0], varPoint.Shape()[1]}});
            varCell.SetSelection({{0, 0}, {varCell.Shape()[0], varCell.Shape()[1]}});
            varNormal.SetSelection({{0, 0}, {varNormal.Shape()[0], varNormal.Shape()[1]}});

            reader.Get<double>(varPoint, points);
            reader.Get<int>(varCell, cells);
            reader.Get<double>(varNormal, normals);
        }

        reader.Get<int>(varStep, &step);

        reader.EndStep();

#ifdef ENABLE_TIMERS
        double time_read = timer_read.stop();
        MPI_Barrier(comm);
        timer_compute.start();
#endif

        vtkSmartPointer<vtkPolyData> polyData = read_mesh(points, cells, normals);
        compute_curvature(polyData);

        if (!rank)
        {
            std::cout << "compute_curvature at step " << step << std::endl;
        }

#ifdef ENABLE_TIMERS
        double time_compute = timer_compute.stop();
        double time_step = timer_total.stop();
        MPI_Barrier(comm);

        log << step << "\t" << time_step << "\t" << time_read << "\t" << time_compute << std::endl;
#endif
    }

#ifdef ENABLE_TIMERS
    log << "total\t" << timer_total.elapsed() << "\t" << timer_read.elapsed() << "\t"
        << timer_compute.elapsed() << "\t" << std::endl;

    log.close();
#endif

    writer.Close();
    reader.Close();
}
