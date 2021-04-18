X = linspace(0, 100, 10);
Y = rand(1, 10) * 100;
AxesH = axes;
plot(X,Y);
XL = get(AxesH, 'XLim');
YL = get(AxesH, 'YLim');
set(AxesH, 'XTick', XL(1):10:XL(2), ...
           'YTick', YL(1):10:YL(2));