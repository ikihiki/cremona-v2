#r "nuget: Fake.Core.Target"
// include Fake modules, see Fake modules section

System.Environment.GetCommandLineArgs()
|> Array.skip 2 // skip fsi.exe; build.fsx
|> Array.toList
|> Fake.Core.Context.FakeExecutionContext.Create false "build.fsx"
|> Fake.Core.Context.RuntimeContext.Fake
|> Fake.Core.Context.setExecutionContext


open Fake.Core
open Fake.Core.TargetOperators

// *** Define Targets ***
Target.create "Hello" (fun _ -> printfn "hello from FAKE!")

Target.create "BuildKernel" (fun _ -> 
  System.IO.File.Copy(
    System.IO.Path.GetFullPath(@".\driver\.devcontainer\.config"),
    System.IO.Path.GetFullPath(@".\infra\kernel\.config"),
    true )

  CreateProcess.fromRawCommand "docker" [ "build"; @".\infra\kernel"; "-t"; "ikihiki/build-kernel:latest" ] 
  |> Proc.run 
  |> fun (result: ProcessResult<unit>) -> if result.ExitCode <> 0 then failwith "Command failed"
 

  let buildpath = System.IO.Path.GetFullPath(@".\dest")

  CreateProcess.fromRawCommand "docker" [ "run"; "--rm"; "-t"; "-v"; $"{buildpath}:/out" ; "ikihiki/build-kernel:latest" ] 
  |> Proc.run 
  |> fun (result: ProcessResult<unit>) -> if result.ExitCode <> 0 then failwith "Command failed"
)

Target.create "BuildDriver" (fun _ -> 

  CreateProcess.fromRawCommand "docker" [ "build"; @".\driver"; "-t"; "ikihiki/build-cremona:latest" ] 
  |> Proc.run 
  |> fun result -> if result.ExitCode <> 0 then failwith "Command failed"


  let buildpath = System.IO.Path.GetFullPath(@".\dest")

  CreateProcess.fromRawCommand "docker" [ "run"; "--rm"; "-t"; "-v"; $"{buildpath}:/out" ; "ikihiki/build-cremona:latest" ] 
  |> Proc.run 
  |> fun (result: ProcessResult<unit>) -> if result.ExitCode <> 0 then failwith "Command failed"
)

Target.create "BuildInitramfs" (fun _ -> 
  System.IO.File.Copy(
    System.IO.Path.GetFullPath(@".\dest\cremona.ko"),
    System.IO.Path.GetFullPath(@".\infra\initramfs\cremona.ko"),
    true )

  CreateProcess.fromRawCommand "docker" [ "build"; @".\infra\initramfs"; "-t"; "ikihiki/build-initramfs:latest" ] 
  |> Proc.run 
  |> fun (result: ProcessResult<unit>) -> if result.ExitCode <> 0 then failwith "Command failed"

  let buildpath = System.IO.Path.GetFullPath(@".\dest")

  CreateProcess.fromRawCommand "docker" [ "run"; "--rm"; "-t"; "-v"; $"{buildpath}:/out" ; "ikihiki/build-initramfs:latest" ] 
  |> Proc.run 
  |> fun (result: ProcessResult<unit>) -> if result.ExitCode <> 0 then failwith "Command failed"

)

"BuildDriver"
  ==> "BuildInitramfs"

Target.create "BuildQemu" (fun _ -> 
  CreateProcess.fromRawCommand "docker" [ "build"; @".\infra\qemu"; "-t"; "ikihiki/qemu:latest" ] 
  |> Proc.run 
  |> fun (result: ProcessResult<unit>) -> if result.ExitCode <> 0 then failwith "Command failed"
)

Target.create "Default" (fun _ ->
  printf "default"
)

"BuildInitramfs"
  ==> "Default"

"BuildKernel"
  ==> "Default"


// *** Start Build ***
Target.runOrDefault "BuildKernel"
