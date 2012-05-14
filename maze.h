#ifndef MAZE_H
#define MAZE_H
#include <vector>
#include <fstream>
#include <iostream>
#include <math.h>
using namespace std;

//simple point class
class Point
{
    public:
        Point(float x1,float y1)
        {
            x=x1;
            y=y1;
        }
		
        Point()
        {
        }
		
        Point(const Point& k)
        {
            x=k.x;
            y=k.y;
        }
		
    void fromfile(ifstream& file)
    {
        file >> x;
        file >> y;
    }
    
	//determine angle of vector defined by (0,0)->This Point
    float angle()
    {
        if(x==0.0)
        {
            if(y>0.0) return 90.0;
            return 270.0;
        }
        float ang=atan(y/x)/3.1415926*180.0;
        
		if(isnan(ang))
			cout << "NAN in angle\n";
        //quadrant 1 or 4
        if(x>0.0)
        {
            return ang;
        }
        return ang+180.0;             
    }
    
	//rotate this point around another point
    void rotate(float angle,Point p)
    {
        float rad=angle/180.0*3.1415926;
        x-=p.x;
        y-=p.y;
        
        float ox=x;
        float oy=y;
        x=cos(rad)*ox-sin(rad)*oy;
        y=sin(rad)*ox+cos(rad)*oy;
        
        x+=p.x;
        y+=p.y;
    }
	//distance between this point and another point
    float distance(Point b)
    {
        float dx=b.x-x;
        float dy=b.y-y;
        return sqrt(dx*dx+dy*dy);
    }
    float x;
    float y;
};

//simple line segment class, used for maze walls
class Line
{
    public:
    Line(Point k,Point j)
    {
        a.x=k.x;
        a.y=k.y;
        b.x=j.x;
        b.y=j.y;
    }
    Line(ifstream& file)
    {
        a.fromfile(file);
        b.fromfile(file);
    }
    Line()
    {
    }
	//midpoint of the line segment
    Point midpoint()
    {
        Point newpoint;
        newpoint.x=(a.x+b.x)/2.0;
        newpoint.y=(a.y+b.y)/2.0;
        return newpoint;
    }
 
	//return point of intersection between two line segments if it exists 
    Point intersection(Line L,bool &found)
    {
            
        Point pt(0.0,0.0);
        Point A(a);
        Point B(b);
        Point C(L.a);
        Point D(L.b);
        
    
        float rTop = (A.y-C.y)*(D.x-C.x)-(A.x-C.x)*(D.y-C.y);
    	float rBot = (B.x-A.x)*(D.y-C.y)-(B.y-A.y)*(D.x-C.x);
    
    	float sTop = (A.y-C.y)*(B.x-A.x)-(A.x-C.x)*(B.y-A.y);
    	float sBot = (B.x-A.x)*(D.y-C.y)-(B.y-A.y)*(D.x-C.x);
    
    	if ( (rBot == 0) || (sBot == 0))
    	{
    		//lines are parallel
    		found = false;
    		return pt;
    	}
    
    	float r = rTop/rBot;
    	float s = sTop/sBot;
    
    	if( (r > 0) && (r < 1) && (s > 0) && (s < 1) )
        {
            
      	pt.x = A.x + r * (B.x - A.x);
      	pt.y = A.y + r * (B.y - A.y);
    
        found=true;
        return pt;
        }
    
    	else
        {
    
        found=false;
        return pt;
        }

    }
    
	//distance between line segment and point
    float distance(Point n)
    {
        float utop = (n.x-a.x)*(b.x-a.x)+(n.y-a.y)*(b.y-a.y);
        float ubot = a.distance(b);
        ubot*=ubot;
		if(ubot==0.0)
		{
			cout << "Ubot zero?" << endl;
			return 0.0;
		}
        float u = utop/ubot;
        
        if(u<0 || u>1)
        {
            float d1=a.distance(n);
            float d2=b.distance(n);
            if(d1<d2) return d1;
            return d2;
        }
        Point p;
        p.x=a.x+u*(b.x-a.x);
        p.y=a.y+u*(b.y-a.y);
        return p.distance(n);
    }
    
	//line segment length
    float length()
    {
        return a.distance(b);
    }
    Point a;
    Point b;
};

//class for the maze navigator
class Character
{
    public:
        vector<float> rangeFinderAngles; //angles of range finder sensors
        vector<float> radarAngles1; //beginning angles for radar sensors
        vector<float> radarAngles2; //ending angles for radar sensors

        vector<float> radar; //stores radar outputs
        vector<float> rangeFinders; //stores rangefinder outputs
        Point location;
        float heading;
        float speed;
        float ang_vel;
        float radius;
        float rangefinder_range;
        
        Character()
        {
            heading=0.0f;
            speed=0.0f;
            ang_vel=0.0f;
            radius=8.0f;
            rangefinder_range=100.0f;
 
			//define the range finder sensors			
            rangeFinderAngles.push_back(-90.0f);
            rangeFinderAngles.push_back(-45.0f);
            rangeFinderAngles.push_back(0.0f);
            rangeFinderAngles.push_back(45.0f);
            rangeFinderAngles.push_back(90.0f);
            rangeFinderAngles.push_back(-180.0f);
            
			//define the radar sensors
            radarAngles1.push_back(315.0);
            radarAngles2.push_back(405.0);
            
            radarAngles1.push_back(45.0);
            radarAngles2.push_back(135.0);
            
            radarAngles1.push_back(135.0);
            radarAngles2.push_back(225.0);
            
            radarAngles1.push_back(225.0);
            radarAngles2.push_back(315.0);
            
            for(int i=0;i<(int)rangeFinderAngles.size();i++)
                rangeFinders.push_back(0.0);
            
            for(int i=0;i<(int)radarAngles1.size();i++)
                radar.push_back(0.0);
        }
        
    

};

//all-encompassing environment class
class Environment
{
    public:
        Environment(const Environment &e)
		{
			hero.location = e.hero.location;
			hero.heading = e.hero.heading;
			hero.speed=e.hero.speed;
			hero.ang_vel=e.hero.ang_vel;
			end=e.end;
			for(int i=0;i<(int)e.lines.size();i++)
			{
				Line* x=new Line(*(e.lines[i]));
				lines.push_back(x);
			}
			update_rangefinders(hero);
			update_radar(hero);
			reachgoal=e.reachgoal;
		}
		//initialize environment from maze file
        Environment(const char* filename)
        {
            ifstream inpfile(filename);
            int num_lines; 
            inpfile >> num_lines; //read in how many line segments
            hero.location.fromfile(inpfile); //read initial location
            inpfile >> hero.heading; //read initial heading
            end.fromfile(inpfile); //read goal location
	    	reachgoal=0;
			//read in line segments
            for(int i=0;i<num_lines;i++)
            {
                Line* x=new Line(inpfile); 
                lines.push_back(x);
            }
			//update sensors
            update_rangefinders(hero);
            update_radar(hero);
        }
        
		//debug function
        void display()
        {
            cout << "Hero: " << hero.location.x << " " << hero.location.y << endl;
            cout << "EndPoint: " << end.x << " " << end.y << endl;
            cout << "Lines:" << endl;
            for(int i=0;i<(int)lines.size();i++)
            {
            cout << lines[i]->a.x << " " << lines[i]->a.y << " " << lines[i]->b.x << " " << lines[i]->b.y << endl;
            }
        }
		
		//used for fitness calculations
		float distance_to_target()
		{
			float dist=hero.location.distance(end);
			if(isnan(dist))
			{
				cout << "NAN Distance error..." << endl;
				return 500.0;
			}
			if(dist<5.0) reachgoal=1; //if within 5 units, success!
			return dist;
		}	
		
		//create neural net inputs from sensors
		void generate_neural_inputs(double* inputs)
		{			
			//bias
			inputs[0]=(1.0);
			
			//rangefinders
			int i;
			for(i=0;i<(int)hero.rangeFinders.size();i++)
			{
				inputs[1+i]=(hero.rangeFinders[i]/hero.rangefinder_range);
				if(isnan(inputs[1+i]))
					cout << "NAN in inputs" << endl;
			}
			
			//radar
			for(int j=0;j<(int)hero.radar.size();j++)
			{
				inputs[i+j]=(hero.radar[j]);
				if(isnan(inputs[i+j]))
					cout << "NAN in inputs" << endl;
			}
			
			return;
		}
		
		//transform neural net outputs into angular velocity and speed
		void interpret_outputs(float o1,float o2)
		{
			if(isnan(o1) || isnan(o2))
				cout << "OUTPUT ISNAN" << endl;	
			
			hero.ang_vel+=(o1-0.5)*1.0;
			hero.speed+=(o2-0.5)*1.0;

			//constraints of speed & angular velocity
			if(hero.speed>3.0) hero.speed=3.0;
			if(hero.speed<-3.0) hero.speed=(-3.0);
			if(hero.ang_vel>3.0) hero.ang_vel=3.0;
			if(hero.ang_vel<-3.0) hero.ang_vel=(-3.0);
		}
        
		//run a time step of the simulation
        void Update()
        {
		if (reachgoal)
			return;
            float vx=cos(hero.heading/180.0*3.1415926)*hero.speed;
            float vy=sin(hero.heading/180.0*3.1415926)*hero.speed;
			if(isnan(vx))
				cout << "VX NAN" << endl;
            
            hero.heading+=hero.ang_vel;
			if(isnan(hero.ang_vel))
				cout << "HERO ANG VEL NAN" << endl;
			
	    if(hero.heading>360) hero.heading-=360;
	    if(hero.heading<0) hero.heading+=360;

            Point newloc;
            newloc.x=vx+hero.location.x;
            newloc.y=vy+hero.location.y;

            //collision detection
            if(!collide_lines(newloc,hero.radius))
            {
                hero.location.x=newloc.x;
                hero.location.y=newloc.y;
            }
			
            update_rangefinders(hero);
            update_radar(hero);
        }
        
		//see if navigator has hit anything
        bool collide_lines(Point loc,float rad)
        {
            for(int i=0;i<(int)lines.size();i++)
            {
                if(lines[i]->distance(loc)<rad)
                    return true;
            }
            return false;
        }
        
		//rangefinder sensors
        void update_rangefinders(Character& h)
        {
			//iterate through each sensor
            for(int i=0;i<(int)h.rangeFinderAngles.size();i++)
            {
                float rad=h.rangeFinderAngles[i]/180.0*3.1415926; //radians...
                
				//project a point from the hero's location outwards
                Point proj_point(h.location.x+cos(rad)*h.rangefinder_range,
                                 h.location.y+sin(rad)*h.rangefinder_range);
				
				//rotate the project point by the hero's heading
				proj_point.rotate(h.heading,h.location);
				
                //create a line segment from the hero's location to projected
				Line projected_line(h.location,proj_point);
				
                float range=h.rangefinder_range; //set range to max by default
				
				//now test against the environment to see if we hit anything
                for(int j=0;j<(int)lines.size();j++)
                {
                    bool found=false;
                    Point intersection=lines[j]->intersection(projected_line,found);
                    if(found)
                    {
						//if so, then update the range to the distance
                        float found_range = intersection.distance(h.location);
			
						//we want the closest intersection
                        if(found_range<range)
                            range=found_range; 
                    }
                }
				if(isnan(range))
					cout << "RANGE NAN" << endl;
                h.rangeFinders[i]=range;
            }
        }
        
		//radar sensors
        void update_radar(Character& h)
        {
            Point target=end; 
			
			//rotate goal with respect to heading of navigator
            target.rotate(-h.heading,h.location);
            
			//translate with respect to location of navigator
			target.x-=h.location.x;
            target.y-=h.location.y;
			
			//what angle is the vector between target & navigator
            float angle=target.angle();
            
			//fire the appropriate radar sensor
            for(int i=0;i<(int)h.radarAngles1.size();i++)
            {
                h.radar[i]=0.0;
            
                if(angle>=h.radarAngles1[i] && angle<h.radarAngles2[i])
                    h.radar[i]=1.0;
                
                if(angle+360.0>=h.radarAngles1[i] && angle+360.0<h.radarAngles2[i])
                    h.radar[i]=1.0;  
            }
        }
        
        ~Environment()
        {
            //clean up lines!
            for(int i=0;i<(int)lines.size();i++)
                delete lines[i];
        }
		
        vector<Line*> lines; //maze line segments
        Character hero; //navigator
        Point end; //the goal
	int reachgoal;
};
#endif
