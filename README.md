# SDL2-circles-collisions
 demo showing SDL2 collisions with falling circles

![image](https://github.com/xp5-org/SDL2-circles-collisions/assets/18539839/74f5eb14-3e1f-4022-88e3-8cb68b4210b0)


to build this, im using the following with nix to launch a shell with the required packages 

``` bash
nix-shell -p SDL SDL2 SDL2_ttf libGL libGLU freeglut glew --run '
    echo "Running G++ compile command"
    bash -c "g++ bubbles.cpp -o bubbles -lSDL2 -lSDL2_ttf -lGL -lGLU -lglut -ldl -lGLEW" > >(tee -a output.log) 2>&1
    echo "Starting compiled program"
    bash -c "./bubbles"
    '
 ```
