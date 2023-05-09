$scripts_prefixes = "invert", "lpf"

foreach ($prefix in $scripts_prefixes) {
    
    g++ $PSScriptRoot\$($prefix)_seq.cpp -o $PSScriptRoot\..\..\build-Mini-Photoshop-Desktop_x86_windows_msys_pe_64bit-Debug\debug\bin\$($prefix)_seq.exe
    g++ $PSScriptRoot\$($prefix)_omp.cpp -fopenmp -o $PSScriptRoot\..\..\build-Mini-Photoshop-Desktop_x86_windows_msys_pe_64bit-Debug\debug\bin\$($prefix)_omp.exe
    g++ $PSScriptRoot\$($prefix)_mpi.cpp -I $env:MSMPI_INC\ -L $env:MSMPI_LIB64\ -lmsmpi -o $PSScriptRoot\..\..\build-Mini-Photoshop-Desktop_x86_windows_msys_pe_64bit-Debug\debug\bin\$($prefix)_mpi.exe
    
}

Write-Output OK