fid=fopen('D:\Thesis\datasets\j2\j2.txt');
s=textscan(fid,'%d %d');
fclose(fid);
x=s{1};
y=s{2};

%{Partition File}%
fid=fopen('D:\Thesis\datasets\j2\j2-pa.txt');
s=textscan(fid,'%d');
fclose(fid);
pa=s{1};
arrayx = zeros(numel(x),2);

paritionnumber = 1;
centroid = [5000,5000];
j = 1;

for i=1:numel(x)
    if pa(i)== paritionnumber
       arrayx(j,1) = x(i);
       arrayx(j,2) = y(i);
       j = j+1;
    end
end
j=j-1;
totaldistance = 0;

fid = fopen('D:\Thesis\datasets\j2\partition1.txt','wt');
for i = 1:j
    fprintf(fid,'%d\t%d\n',arrayx(i,1),arrayx(i,2));
    totaldistance = totaldistance + sqrt((arrayx(i,1)-centroid(1))^2 + (arrayx(i,2)-centroid(2))^2);
end
meandistance = totaldistance/j;
fprintf(fid,'Mean Distance = \t%d\n',meandistance);
fclose(fid);