% Read ToF Radar Data from File
filePath = 'ToF_Radar2.txt'; 
data = readmatrix(filePath); % Reads numeric data 

%X,Y,Z Data 
X = data(:,1); 
Y = data(:,2); 
Z = data(:,3); 

%Sets max level of layers to keep image fitted
RPL = 32;
numLayers = floor(length(X) /RPL);

figure;

hold on; 

%Loop connects data points with line to create clear 3D visuals
for layer = 1:numLayers

    startIdx = (layer - 1) * RPL + 1;
    endIdx = layer * RPL;


    X_layer = X(startIdx:endIdx);
    Y_layer = Y(startIdx:endIdx);
    Z_layer = Z(startIdx:endIdx);


    plot3(X_layer, Y_layer, Z_layer, '-o', 'LineWidth', 2, 'MarkerSize', 1, 'MarkerFaceColor', 'r','color','r');


    X_first = X_layer(1);
    Y_first = Y_layer(1);
    Z_first = Z_layer(1);

    X_last = X_layer(end);
    Y_last = Y_layer(end);
    Z_last = Z_layer(end);


    plot3([X_first, X_last], [Y_first, Y_last], [Z_first, Z_last], '-o', 'LineWidth', 2, 'MarkerSize', 1, 'MarkerFaceColor', 'r', 'color','r');
end


xlabel('X Axis');
ylabel('Y Axis');
zlabel('Z Axis');
title('ToF Radar Point Cloud (Layered with Connections to First and Last Point)');

%Inital View of 3D Image
view(-60, 25); 

grid on;
axis equal;
hold off;
