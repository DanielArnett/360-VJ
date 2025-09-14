# 360-VJ: Reprojection

360-VJ, now powered by the Reprojection shader, is a single effect that can convert between equirectangular, fisheye, and flat projections in both directions. It's a simple way to rotate and convert to/from 360 content. Enjoy!


## Building

The easiest way to build this project, clone the [resolume/ffgl](https://github.com/resolume/ffgl) repo, navigate to the [AddSubtract](https://github.com/resolume/ffgl/tree/master/source/plugins/AddSubtract) plugin example, and replace those files with the ones contained in this repo, such that

Reprojection.cpp -> AddSubtract.cpp

Reprojection.h -> AddSubtract.h


Also note if doing so that the first line of Reprojection.cpp will need changed to read

```
#include <AddSubtract.h>
```

I keep all build artifacts out of this repo so we can use Joris De Jong's CI/CD pipeline to build this plugin.
