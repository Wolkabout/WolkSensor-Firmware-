**IMPORTANT NOTICE:**

Follow these steps in order to download and use *WolkSensor-python-lib*:

1.Clone or download *WolkSensor-Firmware-* project

    git clone https://github.com/Wolkabout/WolkSensor-Firmware-.git

Along with the main project, the submodule project is also downloaded with this *clone* command.

2. Looking at the folder where your downloaded *WolkSensor-Firmware-* project takes place, you will also see a *WolkSensor-python-lib* folder which is **empty**

3.In order to use this submodule, it must first be initialized with the following command:

    git submodule init

4.In order to obtain all data from the submodule project, use the following command:

    git submodule update

The submodule code from the *master* branch is cloned.
After these three obligatory steps, all files and folders from the submodule project appear and they are ready to be used with the main project.

*WolkSensor Firmware* project folder should now look like this:

![untitled](https://user-images.githubusercontent.com/24429196/27535962-39cfed08-5a6d-11e7-8dfa-f10ba983203c.png)

From this point on, after a successful setup, functional tests can be conducted. Tests are located in the functional folder:

    WolkSensor-Firmware-/wolksensor/tests/functional

Within the *functional* folder, you will find *initialization* and *tests* folders, along with a python module named *functional.py*. The above mentioned three tests (data driven, flow and robustness) are located in the *tests* folder. However, running the tests means running the *functional.py* script through the Command Line when located in the *functional* folder.

    python functional.py

Upon successfully finished tests, the following appears:

![testpassed](https://user-images.githubusercontent.com/24429196/27536683-8b19d086-5a70-11e7-80f1-afc8d0dd8b06.png)
