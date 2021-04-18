# Using densities to detect nested clusters

Clustering is one of the most widely used applications in machine learning. It finds use in applications like recommendation systems and fraud detection systems. Similar objects are grouped together to form a cluster. There are situations where there is a need to find a nested cluster, which means finding cluster contained inside another cluster. Results of a density- based algorithm, capable of detecting nested clusters are studied in this thesis. This is a centroid based algorithm. It is built on top of k-means algorithm. The concept of weighted centroid is introduced. The centroid of each cluster is weighted between 0 and 1. Centroid weights are calculated from the density of their clusters. Weight of a centroid is directly proportional to the density of its cluster. Centroids with higher weights have denser clusters. They attract points from a lesser circumference and centroids with lesser weights attract points from a higher circumference. In this way, smaller cluster inside a bigger cluster can be detected. Once the clusters are formed, their densities are calculated again and from the density, weight of every centroid is calculated. Since the higher weight centroids attract only nearby points, it can extract nested clusters present inside a bigger cluster. In k-means, the number of clusters must be given as input and the algorithm returns centroids and partitions. Similarly, this density- based algorithm requires the number of clusters in advance and the clustering solution is returned in the form of centroid weights, centroid locations, and partitions.

Given below is a diagram with nested cluster and its partitioning with k-means and density based algorithm.


<p align="center">
  <img width="459" alt="Screen Shot 2021-04-18 at 19 26 22" src="https://user-images.githubusercontent.com/53753875/115154592-fcf0e780-a07b-11eb-8de1-2c4c8ae8da33.png"> </p>
<p align="center"> Using K-means </p>


<p align="center">
  <img width="459" align="center" alt="Screen Shot 2021-04-18 at 19 28 35" src="https://user-images.githubusercontent.com/53753875/115154649-4a6d5480-a07c-11eb-9187-2ee5196179aa.png"> </p>
<p align="center"> Using density based algorithm </p>

# Notes
1. Code is written in pure C language.
2. There is code for displaying and plotting graphs written in matlab.
3. Makefile sould be used to build the C code which is included in this repository.

# Referrences
1. Master's thesis: https://erepo.uef.fi/handle/123456789/22276
2. Rezaei, M., & Franti, P. (2016). Set matching measures for external cluster validity. IEEE transactions on knowledge and data engineering, 28(8), 2173–2186. doi: 10.1109/tkde.2016.2551240
3. Fränti, P., & Sieranoja, S. (2019). How much can k-means be improved by using better initialization and repeats? Pattern Recognition, 93, 95–112. doi: 10.1016/j.patcog.2019.04.014
4. Fränti, P. and J. Kivijärvi, "Random swapping technique for improving clustering in unsupervised classification", 11th Scandinavian conf. on image analysis
(SCIA'99), Kangerlussuaq, Greenland, 407-413, 1999.
5. Fränti, P., & Sieranoja, S. (2018). K-means properties on six clustering benchmark datasets. Applied intelligence, 48(12), 4743–4759. doi: 10.1007/s10489-018-1238-7
6. Sieranoja, S., & Fränti, P. (2019). Fast and general density peaks clustering. Pattern recognition letters, 128, 551–558. doi: 10.1016/j.patrec.2019.10.019
7. Fränti, P., Rezaei, M., & Zhao, Q. (2014). Centroid index: Cluster level similarity measure. Pattern Recognition, 47(9), 3034–3045. https://doi.org/10.1016/j.patcog.2014.03.017
