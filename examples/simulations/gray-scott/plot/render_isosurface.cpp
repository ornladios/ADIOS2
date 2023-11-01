/*
 * Distributed under the OSI-approved Apache License, Version 2.0.  See
 * accompanying file Copyright.txt for details.
 *
 * Visualization code for the Gray-Scott simulation.
 * Reads and renders iso-surface mesh data.
 *
 * Keichi Takahashi <keichi@is.naist.jp>
 *
 */

#include <iostream>

#include <adios2.h>

#include <vtkActor.h>
#include <vtkAutoInit.h>
#include <vtkCallbackCommand.h>
#include <vtkCellArray.h>
#include <vtkDoubleArray.h>
#include <vtkInteractorStyleSwitch.h>
#include <vtkPointData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkProperty.h>
#include <vtkRenderView.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkSmartPointer.h>

VTK_MODULE_INIT(vtkRenderingOpenGL2);

typedef struct
{
    vtkRenderView *renderView;
    vtkPolyDataMapper *mapper;
    adios2::IO *inIO;
    adios2::Engine *reader;
} Context;

vtkSmartPointer<vtkPolyData> read_mesh(const std::vector<double> &bufPoints,
                                       const std::vector<int> &bufCells,
                                       const std::vector<double> &bufNormals)
{
    int nPoints = bufPoints.size() / 3;
    int nCells = bufCells.size() / 3;

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

void timer_func(vtkObject *object, unsigned long eid, void *clientdata, void *calldata)
{
    Context *context = static_cast<Context *>(clientdata);

    std::vector<double> points;
    std::vector<int> cells;
    std::vector<double> normals;
    int step;

    adios2::StepStatus status = context->reader->BeginStep();

    if (status != adios2::StepStatus::OK)
    {
        return;
    }

    auto varPoint = context->inIO->InquireVariable<double>("point");
    auto varCell = context->inIO->InquireVariable<int>("cell");
    auto varNormal = context->inIO->InquireVariable<double>("normal");
    auto varStep = context->inIO->InquireVariable<int>("step");

    if (varPoint.Shape().size() > 0 || varCell.Shape().size() > 0)
    {
        varPoint.SetSelection({{0, 0}, {varPoint.Shape()[0], varPoint.Shape()[1]}});
        varCell.SetSelection({{0, 0}, {varCell.Shape()[0], varCell.Shape()[1]}});
        varNormal.SetSelection({{0, 0}, {varNormal.Shape()[0], varNormal.Shape()[1]}});

        context->reader->Get<double>(varPoint, points);
        context->reader->Get<int>(varCell, cells);
        context->reader->Get<double>(varNormal, normals);
    }

    context->reader->Get<int>(varStep, &step);

    context->reader->EndStep();

    std::cout << "render_isosurface at step " << step << std::endl;

    vtkSmartPointer<vtkPolyData> polyData = read_mesh(points, cells, normals);

    context->mapper->SetInputData(polyData);
    context->renderView->ResetCamera();
    context->renderView->Render();
}

int main(int argc, char *argv[])
{
    MPI_Init(&argc, &argv);

    int rank, procs, wrank;

    MPI_Comm_rank(MPI_COMM_WORLD, &wrank);

    const unsigned int color = 7;
    MPI_Comm comm;
    MPI_Comm_split(MPI_COMM_WORLD, color, wrank, &comm);

    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &procs);

    if (argc < 2)
    {
        if (rank == 0)
        {
            std::cerr << "Too few arguments" << std::endl;
            std::cout << "Usage: render_isosurface input" << std::endl;
        }
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    if (procs != 1)
    {
        if (rank == 0)
        {
            std::cerr << "render_isosurface only supports serial execution" << std::endl;
        }
        MPI_Abort(MPI_COMM_WORLD, -1);
    }

    const std::string input_fname(argv[1]);

    adios2::ADIOS adios("adios2.xml", comm);

    adios2::IO inIO = adios.DeclareIO("IsosurfaceOutput");
    adios2::Engine reader = inIO.Open(input_fname, adios2::Mode::Read);

    auto mapper = vtkSmartPointer<vtkPolyDataMapper>::New();

    auto actor = vtkSmartPointer<vtkActor>::New();
    actor->SetMapper(mapper);

    auto renderView = vtkSmartPointer<vtkRenderView>::New();
    renderView->GetRenderer()->AddActor(actor);
    renderView->Update();

    auto style = vtkSmartPointer<vtkInteractorStyleSwitch>::New();
    style->SetCurrentStyleToTrackballCamera();

    auto interactor = renderView->GetInteractor();
    interactor->Initialize();
    interactor->SetInteractorStyle(style);
    interactor->CreateRepeatingTimer(100);

    Context context = {
        .renderView = renderView,
        .mapper = mapper,
        .inIO = &inIO,
        .reader = &reader,
    };

    auto timerCallback = vtkSmartPointer<vtkCallbackCommand>::New();
    timerCallback->SetCallback(timer_func);
    timerCallback->SetClientData(&context);
    interactor->AddObserver(vtkCommand::TimerEvent, timerCallback);

    renderView->Render();
    interactor->Start();

    reader.Close();
}
