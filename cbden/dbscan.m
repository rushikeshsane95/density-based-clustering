fid=fopen('/Users/rushikeshsane/Documents/Thesis/j2/j2.txt');
s=textscan(fid,'%d %d');
fclose(fid);
x=s{1};
y=s{2};

X=double(x);
Y=double(y);
Z=[X;Y];
idx = dbscan(Z,1,5, 'Distance','squaredeuclidean'); 
gscatter(Z(:,1),Z(:,2),idx);