//TODO: Change this to be relative to the project
C:\VulkanSDK\1.3.290.0\Bin\glslc.exe ..\Shaders\base_shader.vert -o ..\Shaders\base_shader.vert.spv
C:\VulkanSDK\1.3.290.0\Bin\glslc.exe ..\Shaders\base_shader.frag -o ..\Shaders\base_shader.frag.spv

C:\VulkanSDK\1.3.290.0\Bin\glslc.exe ..\Shaders\Imgui\imgui_shader.vert -o ..\Shaders\Imgui\imgui_shader.vert.spv
C:\VulkanSDK\1.3.290.0\Bin\glslc.exe ..\Shaders\Imgui\imgui_shader.frag -o ..\Shaders\Imgui\imgui_shader.frag.spv

C:\VulkanSDK\1.3.290.0\Bin\glslc.exe ..\Shaders\PostPro\postpro.vert -o ..\Shaders\PostPro\postpro.vert.spv

C:\VulkanSDK\1.3.290.0\Bin\glslc.exe ..\Shaders\PostPro\postpro.frag -o ..\Shaders\PostPro\postpro.frag.spv
C:\VulkanSDK\1.3.290.0\Bin\glslc.exe ..\Shaders\PostPro\DownSample.frag -o ..\Shaders\PostPro\DownSample.frag.spv
C:\VulkanSDK\1.3.290.0\Bin\glslc.exe ..\Shaders\PostPro\UpSample.frag -o ..\Shaders\PostPro\UpSample.frag.spv

C:\VulkanSDK\1.3.290.0\Bin\glslc.exe ..\Shaders\OutputShader\outputshader.frag -o ..\Shaders\OutputShader\outputshader.frag.spv

C:\VulkanSDK\1.3.290.0\Bin\glslc.exe ..\Shaders\ComputeShaders\compute.comp -o ..\Shaders\ComputeShaders\compute.comp.spv

C:\VulkanSDK\1.3.290.0\Bin\glslc.exe --target-env=vulkan1.3 ..\Shaders\RayTracingShaders\sphereint.rint -o  ..\Shaders\RayTracingShaders\sphereint.rint.spv
C:\VulkanSDK\1.3.290.0\Bin\glslc.exe --target-env=vulkan1.3 ..\Shaders\RayTracingShaders\spherehit.rchit -o  ..\Shaders\RayTracingShaders\spherehit.rchit.spv

C:\VulkanSDK\1.3.290.0\Bin\glslc.exe --target-env=vulkan1.3 ..\Shaders\RayTracingShaders\closesthit.rchit -o  ..\Shaders\RayTracingShaders\closesthit.rchit.spv
C:\VulkanSDK\1.3.290.0\Bin\glslc.exe --target-env=vulkan1.3 ..\Shaders\RayTracingShaders\miss.rmiss -o  ..\Shaders\RayTracingShaders\miss.rmiss.spv
C:\VulkanSDK\1.3.290.0\Bin\glslc.exe --target-env=vulkan1.3 ..\Shaders\RayTracingShaders\shadow.rmiss -o  ..\Shaders\RayTracingShaders\shadow.rmiss.spv
C:\VulkanSDK\1.3.290.0\Bin\glslc.exe --target-env=vulkan1.3 ..\Shaders\RayTracingShaders\raygen.rgen -o  ..\Shaders\RayTracingShaders\raygen.rgen.spv
C:\VulkanSDK\1.3.290.0\Bin\glslc.exe --target-env=vulkan1.3 ..\Shaders\RayTracingShaders\anyhit.rahit -o  ..\Shaders\RayTracingShaders\anyhit.rahit.spv

pause
