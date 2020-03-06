<?php

namespace Home\Controller;

use Think\Controller;

class WapController extends Controller
{
    public function index()
    {
        $userTable = M('user');
        $userInfo = $userTable->where('id=1')->find();
        $this->assign('userInfo', $userInfo);
        $this->display();
    }

    public function bill()
    {
        $userTable = M('user');
        $userInfo = $userTable->where('id=1')->find();
        $billTable = M('bill');
        $billInfo = $billTable->where('card="' . $userInfo['card'] . '"')->limit(10)->order('id desc')->select();
        $this->assign('billInfo', $billInfo);
        $this->display();
    }

    public function record()
    {
        $userTable = M('user');
        $userInfo = $userTable->where('id=1')->find();
        $recordTable = M('iopool');
        $recordInfo = $recordTable->where('card="' . $userInfo['card'] . '"')->order('id desc')->select();
        $this->assign('recordInfo', $recordInfo);
        $this->display();
    }

    public function renew()
    {
        $userTable = M('user');
        $userInfo = $userTable->where('id=1')->find();
        $iopool = M('iopool');
        $inpool = $iopool->where('card="' . $userInfo['card'] . '"')->order('id desc')->find();
        $outpool = $iopool->where('card="' . $userInfo['card'] . '" AND id > ' . $inpool['id'])->find();
        $reserveTable = M('reserve');
        $reserveInfo = $reserveTable->where('state=1');
        if ($outpool) {
            echo 1;
        }
        // var_dump($iopoolInfo);
        //$this->display();
    }

    public function reserve()
    {
        $this->display();
    }

    public function addReserve()
    {
        $nowtime = time();
        $time = $_POST['time'];
        $area = $_POST['no'];
        $reTable = M('reserve');
        $data['uid'] = 1;
        $data['zone'] = $area;
        $data['time'] = $nowtime;
        $data['no'] = 1;
        $data['retime'] = $nowtime + ($time * 3600);
        if ($reTable->add($data)) {
            $this->success('预约成功', 'index');
        }
    }
}