fid=fopen('/Users/rushikeshsane/Documents/Thesis/datasets/j1/j1.txt');
s=textscan(fid,'%d %d');
fclose(fid);
x=s{1};
y=s{2};
array = zeros(numel(x),2);
for i=1:numel(x)
   array(i,1) = x(i);
   array(i,2) = y(i);
   plot(array(i,1),array(i,2),'.-','color',[0,0,0]+0.5);
   hold on;
end

%{Centroids File}%

fid=fopen("/Users/rushikeshsane/Downloads/n3-all accepted/iter1.txt");
s=textscan(fid,'%d %d');
fclose(fid);
xc=s{1};
yc=s{2};
noofpartitions = numel(xc);
arrayc = zeros(numel(xc),2);
for i=1:numel(xc)
   arrayc(i,1) = xc(i);
   arrayc(i,2) = yc(i);
end
scatter(arrayc(:,1),arrayc(:,2), 60, 'o', 'k', 'filled')
hold on;

%plot(779,1894,'MarkerSize',100)
%scatter(779,1894, 30, 'o','MarkerFaceColor','g');
%{Partition File}%

fid=fopen('/Users/rushikeshsane/Downloads/n3-all accepted/Acceptedtemp_pa1.txt');
s=textscan(fid,'%d');
fclose(fid);
pa = s{1};
partitionvalues = zeros(1,noofpartitions);


for i=1:noofpartitions
   p1array = zeros(numel(x),2);
   partitionvalues(1,i) = i;
   k = 0;
   for j=1:numel(x)
        if pa(j) == i
            k = k+1;
            p1array(k,1) = array(j,1);
            p1array(k,2) = array(j,2);
        end
   end
   
   p2array = zeros(k,2);
   
   %for j=1:numel(x)
   for j=1:k
      %if p1array(j,1) ~= 0
        p2array(j,1) = p1array(j,1);
        p2array(j,2) = p1array(j,2);
      %end
   end
   
   x1=p2array(:,1);
   y1=p2array(:,2);
   k1 = convhull(x1,y1);
   h = plot(x1(k1),y1(k1), 'Color', [0,0,0]+0.5);
   set(h,'LineWidth', 1.6)
end
set(gca,'visible','off')