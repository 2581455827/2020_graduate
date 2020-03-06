<?php

namespace Home\Controller;

use Think\Controller;   //使用ThinkPHP 框架的控制器

class IndexController extends Controller
{
    public function index()
    {
        $this->display();   //显示首页面
    }

    public function inPool()    //入库记录
    {
        $tableIopool = M('iopool'); //实例化进出库记录表
        $ipInfo = $tableIopool->where('type=1')->select();  //查询进库的数据记录
        $this->assign('ipInfo', $ipInfo);  //将查询结果传递给ipInfo变量进行前端显示
        $this->display();   //display()方法进行渲染显示
    }

    public function outPool()   //出库记录
    {
        $tableIopool = M('iopool');
        $ipInfo = $tableIopool->where('type=2')->select();  //查询出库的数据记录
        $this->assign('ipInfo', $ipInfo);
        $this->display();
    }

    public function realTime()  //实时监测车位变化
    {
        $this->display();
    }

    public function bill()  //账单记录
    {
        $billTable=M('bill');
        $billInfo=$billTable->join('user ON bill.card = bill.card')->select();  //使用卡号进行关联查询
        $this->assign('billInfo',$billInfo);
        $this->display();
    }

    public function reserve()
    {
        $reserve = M('reserve');
        $reserveInfo = $reserve->select();
        $this->assign('reserveInfo',$reserveInfo);
        $this->display();
    }

    public function welcome()
    {
        $this->display();
    }

    public function order()
    {
        $this->display();
    }
}