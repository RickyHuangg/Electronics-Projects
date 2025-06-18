% Define the serial port and settings
port = "COM4"; 
baudRate = 115200;
timeout = 10;

% Open the serial port
s = serialport(port, baudRate, 'Timeout', timeout);

disp("Opening: " + port);

flush(s);

% Wait for user to press enter to start communication.
input("Press Enter to start communication...", 's');

% Send 's' to the MCU to start transmission
writeline(s, 's');

angleStep = 11.25;  
maxScans = floor(360 / angleStep); 
maxDepthLayers = 50; 

X = zeros(1, maxScans * maxDepthLayers);
Y = zeros(1, maxScans * maxDepthLayers);
Z = zeros(1, maxScans * maxDepthLayers);

index = 1;
angle = angleStep;  
z = 0;  

% Open file for writing
fileID = fopen("ToF_Radar.txt", "a");

%Creates plots
figure;
hold on;
grid on;
xlabel('X-axis', 'FontSize', 14, 'FontWeight', 'bold');
ylabel('Y-axis', 'FontSize', 14, 'FontWeight', 'bold');
zlabel('Z-axis', 'FontSize', 14, 'FontWeight', 'bold');
title('3D ToF Radar Data', 'FontSize', 16, 'FontWeight', 'bold');

%Set axis limits
xlim([-4000, 4000]); 
ylim([-4000, 4000]); 
zlim([0, 10000]); 

%Grid on to keep same plot
grid on;
set(gca, 'GridAlpha', 0.5, 'LineWidth', 2); 


view([-60, 25]); 

% Set dot size for clear measurements
scatterPlot = scatter3(NaN, NaN, NaN, 80, 'filled');
linePlot = plot3(NaN, NaN, NaN, 'LineWidth', 1.5, 'Color', 'r');  

disp("Receiving data... Press Ctrl+C to stop.");

try
    while true
        if s.NumBytesAvailable > 0
            data = readline(s);
            
            if ~isempty(data)
                
                value = str2double(strtrim(data));
                
                % Check if conversion was successful
                if isnan(value)
                    continue;
                end

                % Trig Calculations
                x = value * sind(angle);
                y = value * cosd(angle);

                % Set X,Y,Z 
                X(index) = x;
                Y(index) = y;
                Z(index) = z;
                index = index + 1;

                
                set(scatterPlot, 'XData', X(1:index-1), 'YData', Y(1:index-1), 'ZData', Z(1:index-1));

                % Connects points
                set(linePlot, 'XData', X(1:index-1), 'YData', Y(1:index-1), 'ZData', Z(1:index-1));

                drawnow;

                % Display results
                fprintf("%.2f, %.2f, %.2f\n", x, y, z);
                
               
                fprintf(fileID, "%.2f, %.2f, %.2f\n", x, y, z);
                
              
                fclose(fileID);
                fileID = fopen("ToF_Radar.txt", "a");
                
                
                if angle >= 360
                    angle = angleStep;  
                    z = z + 100;        
                else
                    angle = angle + angleStep; 
                end
            end
        else
            pause(0.1);
        end
    end
catch ME
    disp("Stopping communication due to an error.");
    disp(ME.message); 
end

% Close the file and serial port
fclose(fileID);
clear s;
disp("Closed serial connection.");
