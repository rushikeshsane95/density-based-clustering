fid=fopen('D:\Thesis\datasets\j2\j2.txt');
s=textscan(fid,'%d %d');
fclose(fid);
x=s{1};
y=s{2};

%{Centroids File}%
fid=fopen('D:\Thesis\datasets\j2\j2-gt.txt');
s=textscan(fid,'%d %d');
fclose(fid);
xc=s{1};
yc=s{2};
noofpartitions = numel(xc);
arraypa = numel(x);
mindistarray = numel(x);
dist = numel(xc);
weight = [0.02,0.42,0.14,0.21,0.14,0.07];
for i=1:numel(x)
    min = 100000000;
    index = 0;
    for j=1:numel(xc)
        dist = weight(j) * ((x(i)-xc(j)).^2 + (y(i)-yc(j)).^2);
        if dist<min
            min = dist;
            index = j;
            mindistarray(i) = dist;
        end
    end
    arraypa(i) = index;
end

fid = fopen('D:\Thesis\datasets\j2\partition.txt','wt');
for i = 1:numel(arraypa)
    fprintf(fid,'%d\n',arraypa(i));
end
fclose(fid);