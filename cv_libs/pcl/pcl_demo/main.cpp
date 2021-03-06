#include <pcl/io/ply_io.h>
#include<pcl/filters/voxel_grid.h>
#include <pcl/visualization/cloud_viewer.h>

#include "extract_domain_plane_ransac.h"

int main(int argc, char **argv)
{
    if(argc != 2) {
        std::cerr << "Usage: extract_domain_plane_ransac <xyz-ply-file>" << std::endl;
        return -1;
    }

    std::string str_file = argv[1];

    pcl::PointCloud<pcl::PointXYZRGB>::Ptr cloud_in(new pcl::PointCloud<pcl::PointXYZRGB>);
    int ret = pcl::io::loadPLYFile(str_file, *cloud_in);
    if(-1 == ret) {
        std::string msg_error = "Couldn't read file " + str_file + "\n";
        PCL_ERROR(msg_error.c_str());
        return -1;
    }

    PointCloudXYZ::Ptr cloud_xyz(new PointCloudXYZ);
    pcl::copyPointCloud(*cloud_in, *cloud_xyz);

    std::cout << "before filter cloud_xyz size: " << cloud_xyz->size() << std::endl;

    PointCloudXYZ::Ptr cloud_after_voxelgrid(new PointCloudXYZ);
    pcl::VoxelGrid<pcl::PointXYZ> voxelgrid; 
    voxelgrid.setInputCloud(cloud_xyz);
    voxelgrid.setLeafSize(0.03f, 0.03f, 0.03f);
    voxelgrid.filter(*cloud_xyz);

    std::cout << "after filter cloud_xyz size: " << cloud_xyz->size() << std::endl;

    PointCloudXYZ::Ptr cloud_out = extract_domain_plane_ransac(cloud_xyz, 0.01);
    
    std::cout << "cloud_out size: " << cloud_out->size() << std::endl;

    // get the plane normal
    Eigen::Vector3f v3normal; 
    Eigen::Vector3f v3centroid;
    pcl::PointCloud<pcl::Normal>::Ptr normals(new pcl::PointCloud<pcl::Normal>); 
    calc_plane_normal(cloud_out, v3normal, v3centroid);
    for(int i=0; i<cloud_out->size(); i++) {
        pcl::Normal pcl_normal(v3normal[0], v3normal[1], v3normal[2]);
        normals->push_back(pcl_normal);
    }
    
    // save & view
    {
        // pcl::io::savePLYFileASCII("/home/cg/Downloads/cg_wall_new.ply", *cloud_xyz);

        boost::shared_ptr<pcl::visualization::PCLVisualizer> viewer(new pcl::visualization::PCLVisualizer("3D Viewer"));

        viewer->initCameraParameters();

        int v1(0);
        viewer->createViewPort(0.0, 0.0, 0.5, 1.0, v1);
        viewer->setBackgroundColor(0, 0, 0, v1);
        pcl::visualization::PointCloudColorHandlerCustom<pcl::PointXYZ> source_color(cloud_xyz, 0, 255, 0);
        pcl::visualization::PointCloudColorHandlerGenericField<pcl::PointXYZ> fildColor(cloud_xyz, "z"); // 按照z字段进行渲染
        // pcl::visualization::PointCloudColorHandlerRGBField<pcl::PointXYZRGB> rgb(cloud_xyz);
        viewer->addPointCloud<pcl::PointXYZ>(cloud_xyz, source_color, "source", v1);
        viewer->setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_POINT_SIZE, 2, "source");

        int v2(0);
        viewer->createViewPort(0.5, 0.0, 1.0, 1.0, v2);
        viewer->setBackgroundColor(0.3, 0.3, 0.3, v2);        
        pcl::visualization::PointCloudColorHandlerCustom<pcl::PointXYZ> target_color(cloud_out, 255, 255, 255);
        viewer->addPointCloud<pcl::PointXYZ>(cloud_out, target_color, "target", v2);
        // 200表示需要显示法向的点云间隔，0.5表示法向长度
        viewer->addPointCloudNormals<pcl::PointXYZ, pcl::Normal>(cloud_out, normals, 200, 0.5, "normals",v2);
        viewer->setPointCloudRenderingProperties(pcl::visualization::PCL_VISUALIZER_POINT_SIZE, 2, "target");

        viewer->addCoordinateSystem(0.5);
        
        Eigen::Affine3f m3A = get_transform_new_from_old(v3normal, v3centroid);
        viewer->addCoordinateSystem(0.5, m3A, v2);
    
        while(!viewer->wasStopped()) {
            viewer->spinOnce(100);
            boost::this_thread::sleep(boost::posix_time::microseconds(100000));
        }

        // viewer->spin();
    }

    return 0;
}